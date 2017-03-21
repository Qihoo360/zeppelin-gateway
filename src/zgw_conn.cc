#include <memory>
#include <sys/time.h>

#include "zgw_conn.h"
#include "zgw_server.h"
#include "zgw_auth.h"
#include "zgw_xml.h"
#include "libzgw/zgw_namelist.h"

#include "slash_hash.h"

extern ZgwServer* g_zgw_server;

static Status ExtraBucketAndObject(const std::string& path,
                                   std::string* bucket_name, std::string* object_name) {
  if (path[0] != '/') {
    return Status::IOError("Path parse error: " + path);
  }
  size_t pos = path.find('/', 1);
  if (pos == std::string::npos) {
    bucket_name->assign(path.substr(1));
    object_name->clear();
  } else {
    bucket_name->assign(path.substr(1, pos - 1));
    object_name->assign(path.substr(pos + 1));
  }
  if (!object_name->empty() && object_name->back() == '/') {
    object_name->erase(object_name->size() - 1);
  }

  return Status::OK();
}

static std::string http_nowtime(time_t t) {
  char buf[100] = {0};
  struct tm t_ = *gmtime(&t);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &t_);
  return std::string(buf);
}

static void DumpHttpRequest(const pink::HttpRequest* req) {
  DLOG(INFO) << "handle get"<< std::endl;
  DLOG(INFO) << " + method: " << req->method;
  DLOG(INFO) << " + path: " << req->path;
  DLOG(INFO) << " + version: " << req->version;
  if (req->content.size() > 50) {
    DLOG(INFO) << " + content: " << req->content.substr(0, 50);
  } else {
    DLOG(INFO) << " + content: " << req->content;
  }
  DLOG(INFO) << " + headers: ";
  DLOG(INFO) << "------------------------------------- ";
  for (auto h : req->headers) {
    DLOG(INFO) << "   + " << h.first << "=>" << h.second;
  }
  DLOG(INFO) << "------------------------------------- ";
  DLOG(INFO) << "------------------------------------- ";
  DLOG(INFO) << " + query_params: ";
  for (auto q : req->query_params) {
    DLOG(INFO) << "   + " << q.first << "=>" << q.second;
  }
  DLOG(INFO) << "------------------------------------- ";
} 

std::string ZgwConn::GetAccessKey() {
  if (!req_->query_params["X-Amz-Credential"].empty()) {
    std::string credential_str = req_->query_params.at("X-Amz-Credential");
    return credential_str.substr(0, 20);
  } else {
    std::string auth_str;
    auto iter = req_->headers.find("authorization");
    if (iter != req_->headers.end())
      auth_str.assign(iter->second);
    else return "";
    size_t pos = auth_str.find("Credential");
    if (pos == std::string::npos)
      return "";
    size_t slash_pos = auth_str.find('/');
    // e.g. auth_str: "...Credential=f3oiCCuyE7v3dOZgeEBsa/20170225/us..."
    return auth_str.substr(pos + 11, slash_pos - pos - 11);
  }
  return "";
}

ZgwConn::ZgwConn(const int fd,
                 const std::string &ip_port,
                 pink::Thread* worker)
      : HttpConn(fd, ip_port) {
  worker_ = reinterpret_cast<ZgwWorkerThread *>(worker);
  store_ = worker_->GetStore();
}

void ZgwConn::DealMessage(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  // DumpHttpRequest(req);

  // Copy req and resp
  req_ = const_cast<pink::HttpRequest *>(req);
  resp_ = resp;

  // Get bucket name and object name
  Status s = ExtraBucketAndObject(req_->path, &bucket_name_, &object_name_);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "HTTP path parse error: " << s.ToString();
    return;
  }

  // Users operation, without authorization for now
  if (req_->method == "GET" &&
      bucket_name_ == "admin_list_users") {
    ListUsersHandle();
    return;
  } else if (req_->method == "PUT" &&
             bucket_name_ == "admin_put_user") {
    if (object_name_.empty()) {
      resp_->SetStatusCode(400);
      return;
    }
    std::string access_key;
    std::string secret_key;
    s = store_->AddUser(object_name_, &access_key, &secret_key);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      resp_->SetBody(s.ToString());
    } else {
      resp_->SetStatusCode(200);
      resp_->SetBody(access_key + "\r\n" + secret_key);
    }
    return;
  }
  
  // Get access key
  access_key_ = GetAccessKey();
  // Authorize access key
  s = store_->GetUser(access_key_, &zgw_user_);
  ZgwAuth zgw_auth;
  if (!s.ok()) {
    resp_->SetStatusCode(403);
    resp_->SetBody(xml::ErrorXml(xml::InvalidAccessKeyId, ""));
    return;
  }

  // TODO (gaodq) disable request authorization
  // Authorize request
  // if (!zgw_auth.Auth(req_, zgw_user_->secret_key(access_key_))) {
  //   resp_->SetStatusCode(403);
  //   resp_->SetBody(xml::ErrorXml(xml::SignatureDoesNotMatch, ""));
  //   return;
  // }

  // Get buckets namelist and ref
  s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name_);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  if (!bucket_name_.empty() && buckets_name_->IsExist(bucket_name_)) {
    // Get objects namelist and ref
    s = g_zgw_server->objects_list()->Ref(store_, bucket_name_, &objects_name_);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "List objects name list failed: " << s.ToString();
      s = g_zgw_server->buckets_list()->Unref(store_, access_key_);
      return;
    }
  }

  METHOD method;
  if (req_->method == "GET") {
    method = kGet;
  } else if (req_->method == "PUT") {
    method = kPut;
  } else if (req_->method == "DELETE") {
    method = kDelete;
  } else if (req_->method == "HEAD") {
    method = kHead;
  } else if (req_->method == "POST") {
    method = kPost;
  } else {
    method = kUnsupport;
  }

  if (bucket_name_.empty()) {
    if (method == kGet) {
      ListBucketHandle();
    } else {
      // Unknow request
      resp_->SetStatusCode(405);
      resp_->SetBody(xml::ErrorXml(xml::MethodNotAllowed, ""));
    }
  } else if (IsBucketOp()) {
    switch(method) {
      case kGet:
        if (req_->query_params.find("uploads") != req_->query_params.end()) {
          ListMultiPartsUpload();
        } else {
          ListObjectHandle();
        }
        break;
      case kPut:
        PutBucketHandle();
        break;
      case kDelete:
        DelBucketHandle();
        break;
      case kHead:
        if (!buckets_name_->IsExist(bucket_name_)) {
          resp_->SetStatusCode(404);
        } else {
          resp_->SetStatusCode(200);
        }
        break;
      case kPost:
        if (req_->query_params.find("delete") != req_->query_params.end()) {
          DelMultiObjectsHandle();
        }
        break;
      default:
        break;
    }
  } else if (IsObjectOp()) {
    // Check whether bucket existed in namelist meta
    if (!buckets_name_->IsExist(bucket_name_)) {
      resp_->SetStatusCode(404);
      resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    } else {
      DLOG(INFO) << "Object Op: " << req_->path << " confirm bucket exist";
      g_zgw_server->object_mutex()->Lock(bucket_name_ + object_name_);
      switch(method) {
        case kGet:
          if (req_->query_params.find("uploadId") != req_->query_params.end()) {
            ListParts(req_->query_params["uploadId"]);
          } else {
            GetObjectHandle();
          }
          break;
        case kPut:
          if (req_->query_params.find("partNumber") != req_->query_params.end() &&
              req_->query_params.find("uploadId") != req_->query_params.end()) {
            UploadPartHandle(req_->query_params["partNumber"],
                             req_->query_params["uploadId"]);
          } else {
            PutObjectHandle();
          }
          break;
        case kDelete:
          if (req_->query_params.find("uploadId") != req_->query_params.end()) {
            AbortMultiUpload(req_->query_params["uploadId"]);
          } else {
            DelObjectHandle();
          }
          break;
        case kHead:
          GetObjectHandle(true);
          break;
        case kPost:
          if (req_->query_params.find("uploads") != req_->query_params.end()) {
            InitialMultiUpload();
          } else if (req_->query_params.find("uploadId") != req_->query_params.end()) {
            CompleteMultiUpload(req_->query_params["uploadId"]);
          }
          break;
        default:
          break;
      }
      g_zgw_server->object_mutex()->Unlock(bucket_name_ + object_name_);
    }
  } else {
    // Unknow request
    resp_->SetStatusCode(501);
    resp_->SetBody(xml::ErrorXml(xml::NotImplemented, ""));
  }

  // Unref namelist
  Status s1 = Status::OK();
  s = g_zgw_server->buckets_list()->Unref(store_, access_key_);
  if (!bucket_name_.empty()) {
    s1 = g_zgw_server->objects_list()->Unref(store_, bucket_name_);
  }
  if (!s.ok() || !s1.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Unref namelist failed: " << s.ToString();
    return;
  }

  resp_->SetHeaders("Date", http_nowtime(time(NULL)));
}

void ZgwConn::InitialMultiUpload() {
  std::string upload_id, internal_obname;

  timeval now;
  gettimeofday(&now, NULL);
  libzgw::ZgwObjectInfo ob_info(now, "", 0, libzgw::kStandard, zgw_user_->user_info());
  std::string tmp_obname = object_name_ + std::to_string(time(NULL));
  upload_id.assign(slash::md5(tmp_obname));
  internal_obname.assign("__" + object_name_ + upload_id);

  libzgw::ZgwObject object(bucket_name_, internal_obname, "", ob_info);
  object.SetUploadId(upload_id);
  Status s = store_->AddObject(object);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    return;
  }
  DLOG(INFO) << "Get upload id, and insert multiupload meta to zp";

  // Insert into namelist
  objects_name_->Insert(internal_obname);
  DLOG(INFO) << "Insert into namelist: " << internal_obname;

  // Success Response
  resp_->SetBody(xml::InitiateMultipartUploadResultXml(bucket_name_,
                                                       object_name_, upload_id));
  resp_->SetStatusCode(200);
}

void ZgwConn::UploadPartHandle(const std::string& part_num, const std::string& upload_id) {
  std::string internal_obname = "__" + object_name_ + upload_id;
  if (!objects_name_->IsExist(internal_obname)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchUpload, upload_id));
    return;
  }

  Status s;
  timeval now;
  gettimeofday(&now, NULL);
  // Handle copy operation
  std::string src_bucket_name, src_object_name, etag;
  bool is_copy_op = !req_->headers["x-amz-copy-source"].empty();
  if (is_copy_op) {
    std::unique_ptr<libzgw::ZgwObject> src_object_p;
    if (!GetSourceObject(src_object_p)) {
      return;
    }
    src_object_p->info().user = zgw_user_->user_info();
    src_object_p->info().mtime = now;
    etag = src_object_p->info().etag;
    s = store_->UploadPart(bucket_name_, internal_obname, src_object_p->info(),
                           src_object_p->content(), std::stoi(part_num));
  } else {
    etag.assign("\"" + slash::md5(req_->content) + "\"");
    libzgw::ZgwObjectInfo ob_info(now, etag, req_->content.size(), libzgw::kStandard,
                                  zgw_user_->user_info());
    s = store_->UploadPart(bucket_name_, internal_obname, ob_info, req_->content,
                           std::stoi(part_num));
  }
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "UploadPart data failed: " << s.ToString();
    return;
  }
  DLOG(INFO) << "UploadPart: " << req_->path << " confirm add to zp success";

  resp_->SetHeaders("ETag", etag);
  resp_->SetStatusCode(200);
}

void ZgwConn::CompleteMultiUpload(const std::string& upload_id) {
  std::string internal_obname = "__" + object_name_ + upload_id;
  if (!objects_name_->IsExist(internal_obname)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchUpload, upload_id));
    return;
  }
  DLOG(INFO) << "CompleteMultiUpload: " << req_->path << " confirm upload id exist";

  // Check every part's etag and part num
  std::vector<std::pair<int, std::string>> recv_parts;
  if (!xml::ParseCompleteMultipartUploadXml(req_->content, &recv_parts) ||
      recv_parts.empty()) {
    resp_->SetStatusCode(400);
    resp_->SetBody(xml::ErrorXml(xml::MalformedXML, ""));
    return;
  }
  std::vector<std::pair<int, libzgw::ZgwObject>> store_parts;
  Status s = store_->ListParts(bucket_name_, internal_obname, &store_parts);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "CompleteMultiUpload failed in list object parts: " << s.ToString();
    return;
  }
  std::set<int> existed_parts;
  for (auto& it : store_parts) {
    existed_parts.insert(it.first);
  }
  if (recv_parts.size() != store_parts.size()) {
    resp_->SetStatusCode(400);
    resp_->SetBody(xml::ErrorXml(xml::InvalidPart, ""));
  }
  for (size_t i = 0; i < recv_parts.size(); i++) {
    // check part num order and existance
    if (existed_parts.find(recv_parts[i].first) == existed_parts.end()) {
      resp_->SetStatusCode(400);
      resp_->SetBody(xml::ErrorXml(xml::InvalidPart, ""));
      return;
    }
    if (recv_parts[i].first != store_parts[i].first) {
      resp_->SetStatusCode(400);
      resp_->SetBody(xml::ErrorXml(xml::InvalidPartOrder, ""));
      return;
    }
    // Check etag
    const libzgw::ZgwObjectInfo& info = store_parts[i].second.info();
    if (info.etag != recv_parts[i].second) {
      resp_->SetStatusCode(400);
      resp_->SetBody(xml::ErrorXml(xml::InvalidPart, ""));
      return;
    }
  }

  // Delete old object data
  if (objects_name_->IsExist(object_name_)) {
    s = store_->DelObject(bucket_name_, object_name_);
    if (!s.ok() && !s.IsNotFound()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "CompleteMultiUpload failed in delete old object: " << s.ToString();
      return;
    }
    DLOG(INFO) << "CompleteMultiUpload: " << req_->path << " confirm delete old object";
  }

  // Update object meta in zp
  std::string final_etag;
  s = store_->CompleteMultiUpload(bucket_name_, internal_obname, store_parts, &final_etag);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "CompleteMultiUpload failed: " << s.ToString();
    return;
  }
  DLOG(INFO) << "CompleteMultiUpload: " << req_->path << " confirm zp's objects change name";

  objects_name_->Insert(object_name_);
  objects_name_->Delete(internal_obname);

  resp_->SetStatusCode(200);
  final_etag.erase(final_etag.size() - 1); // erase last '"'
  final_etag += "-" + std::to_string(store_parts.size()) + "\"";
  resp_->SetHeaders("ETag", final_etag);
  resp_->SetBody(xml::CompleteMultipartUploadResultXml(bucket_name_,
                                                       object_name_,
                                                       final_etag));
}

void ZgwConn::AbortMultiUpload(const std::string& upload_id) {
  std::string internal_obname = "__" + object_name_ + upload_id;
  if (!objects_name_->IsExist(internal_obname)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchUpload, upload_id));
    return;
  }

  Status s = store_->DelObject(bucket_name_, internal_obname);
  if (!s.ok()) {
    if (s.IsNotFound()) {
      // But founded in list meta, continue to delete from list meta
    } else {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "AbortMultiUpload failed: " << s.ToString();
      return;
    }
  }

  objects_name_->Delete(internal_obname);
  DLOG(INFO) << "AbortMultiUpload: " << req_->path << " confirm delete object meta from namelist success";

  // Success
  resp_->SetStatusCode(204);
}

void ZgwConn::ListParts(const std::string& upload_id) {
  std::string internal_obname = "__" + object_name_ + upload_id;
  if (!objects_name_->IsExist(internal_obname)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchUpload, upload_id));
    return;
  }
  DLOG(INFO) << "ListParts: " << req_->path << " confirm upload exist";

  std::vector<std::pair<int, libzgw::ZgwObject>> parts;
  Status s = store_->ListParts(bucket_name_, internal_obname, &parts);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "ListParts failed: " << s.ToString();
    return;
  }

  std::map<std::string, std::string> args{
    {"Bucket", bucket_name_},
    {"Key", object_name_},
    {"UploadId", upload_id},
    {"StorageClass", "STANDARD"},
    // {"PartNumberMarker", "1"},
    // {"NextPartNumberMarker", ""},
    {"MaxParts", "1000"},
    {"IsTruncated", "false"},
  };
  resp_->SetStatusCode(200);
  resp_->SetBody(xml::ListPartsResultXml(parts, zgw_user_, args));
}

void ZgwConn::ListMultiPartsUpload() {
  // Check whether bucket existed in namelist meta
  if (!buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    return;
  }
  DLOG(INFO) << "ListMultiPartsUpload: " << req_->path << " confirm bucket exist";

  Status s;
  std::vector<libzgw::ZgwObject> objects;
  {
    std::lock_guard<std::mutex> lock(objects_name_->list_lock);
    for (auto &name : objects_name_->name_list) {
      if (name.find_first_of("__") != 0) {
        continue;
      }
      libzgw::ZgwObject object(bucket_name_, name);
      s = store_->GetObject(&object, false);
      if (!s.ok()) {
        if (s.IsNotFound()) {
          continue;
        }
        resp_->SetStatusCode(500);
        LOG(ERROR) << "ListMultiPartsUpload failed: " << s.ToString();
        return;
      }
      objects.push_back(object);
    }
  }

  std::string next_key_marker = objects.empty() ? "" : objects.back().name();
  std::map<std::string, std::string> args {
    {"Bucket", bucket_name_},
    {"NextKeyMarker", next_key_marker.empty() ? "" :
      next_key_marker.substr(2, next_key_marker.size() - 32 -2)},
    {"NextUploadIdMarker", objects.empty() ? "" : objects.back().upload_id()},
    {"MaxUploads", "1000"},
    {"IsTruncated", "false"},
  };
  resp_->SetStatusCode(200);
  resp_->SetBody(xml::ListMultipartUploadsResultXml(objects, args));
}

void ZgwConn::ListUsersHandle() {
  std::set<libzgw::ZgwUser *> user_list; // name : keys
  Status s = store_->ListUsers(&user_list);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    resp_->SetBody(s.ToString());
  } else {
    resp_->SetStatusCode(200);
    std::string body;
    for (auto &user : user_list) {
      const auto &info = user->user_info();
      body.append("disply_name: " + info.disply_name + "\r\n");

      for (auto &key_pair : user->access_keys()) {
        body.append(key_pair.first + "\r\n"); // access key
        body.append(key_pair.second + "\r\n"); // secret key
      }
      body.append("\r\n");
    }
    resp_->SetBody(body);
  }
}

void ZgwConn::DelMultiObjectsHandle() {
  if (!buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    return;
  }

  std::vector<std::string> keys;
  if (!xml::ParseDelMultiObjectXml(req_->content, &keys)) {
    resp_->SetStatusCode(400);
    resp_->SetBody(xml::ErrorXml(xml::MalformedXML, ""));
  }
  std::vector<std::string> success_keys;
  std::map<std::string, std::string> error_keys;
  Status s;
  for (auto &key : keys) {
    DLOG(INFO) << "DeleteMuitiObjects: " << key;
    if (objects_name_->IsExist(key)) {
      s = store_->DelObject(bucket_name_, key);
      if (!s.ok()) {
        error_keys.insert(std::make_pair(key, "InternalError"));
        continue;
      }
    }
    objects_name_->Delete(key);
    success_keys.push_back(key);
  }
  resp_->SetBody(xml::DeleteResultXml(success_keys, error_keys));
  resp_->SetStatusCode(200);
}

void ZgwConn::DelObjectHandle() {
  DLOG(INFO) << "DeleteObject: " << bucket_name_ << "/" << object_name_;

  // Check whether object existed in namelist meta
  if (!objects_name_->IsExist(object_name_)) {
    resp_->SetStatusCode(204);
    return;
  }
  DLOG(INFO) << "DelObject: " << req_->path << " confirm object exist";

  // Delete object
  Status s = store_->DelObject(bucket_name_, object_name_);
  if (!s.ok()) {
    if (s.IsNotFound()) {
      // But founded in list meta, continue to delete from list meta
    } else {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "Delete object data failed: " << s.ToString();
      return;
    }
  }
  DLOG(INFO) << "DelObject: " << req_->path << " confirm delete object from zp success";

  // Delete from list meta
  objects_name_->Delete(object_name_);

  DLOG(INFO) << "DelObject: " << req_->path << " confirm delete object meta from namelist success";

  // Success
  resp_->SetStatusCode(204);
}

void ZgwConn::GetObjectHandle(bool is_head_op) {
  DLOG(INFO) << "GetObjects: " << bucket_name_ << "/" << object_name_;

  if (!objects_name_->IsExist(object_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchKey, object_name_));
    return;
  }
  DLOG(INFO) << "GetObject: " << req_->path << " confirm object exist";

  // Get object
  libzgw::ZgwObject object(bucket_name_, object_name_);
  bool need_content = !is_head_op;
  Status s = store_->GetObject(&object, need_content);
  if (!s.ok()) {
    if (s.IsNotFound()) {
      LOG(WARNING) << "Data size maybe strip count error";
    } else {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "Get object data failed: " << s.ToString();
      return;
    }
  }
  DLOG(INFO) << "GetObject: " << req_->path << " confirm get object from zp success";

  resp_->SetBody(object.content());
  resp_->SetStatusCode(200);
}

bool ZgwConn::GetSourceObject(std::unique_ptr<libzgw::ZgwObject>& src_object_p) {
  std::string src_bucket_name, src_object_name;
  auto& source = req_->headers.at("x-amz-copy-source");
  Status s = ExtraBucketAndObject(source, &src_bucket_name, &src_object_name);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Path parse error: " << s.ToString();
    return false;
  }
  if (src_bucket_name.empty() || src_object_name.empty()) {
    resp_->SetStatusCode(400);
    resp_->SetBody(xml::ErrorXml(xml::InvalidArgument, "x-amz-copy-source"));
    return false;
  }
  if (!buckets_name_->IsExist(src_bucket_name)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, src_bucket_name));
    return false;
  }

  libzgw::NameList *tmp_obnames = NULL;
  s = g_zgw_server->objects_list()->Ref(store_, src_bucket_name, &tmp_obnames);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Ref objects name list failed: " << s.ToString();
    return false;
  }
  if (tmp_obnames == NULL || !tmp_obnames->IsExist(src_object_name)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchKey, src_object_name));
    g_zgw_server->objects_list()->Unref(store_, src_bucket_name);
    return false;
  }
  g_zgw_server->objects_list()->Unref(store_, src_bucket_name);

  // Get source object
  src_object_p.reset(new libzgw::ZgwObject(src_bucket_name, src_object_name));
  s = store_->GetObject(src_object_p.get(), true);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Put copy object data failed: " << s.ToString();
    return false;
  }

  return true;
}

void ZgwConn::PutObjectHandle() {
  DLOG(INFO) << "PutObjcet: " << bucket_name_ << "/" << object_name_;

  Status s;
  timeval now;
  gettimeofday(&now, NULL);
  std::string etag;
  // Handle copy operation
  bool is_copy_op = !req_->headers["x-amz-copy-source"].empty();
  if (is_copy_op) {
    std::unique_ptr<libzgw::ZgwObject> src_object_p;
    if (!GetSourceObject(src_object_p)) {
      return;
    }
    src_object_p->SetBucketName(bucket_name_);
    src_object_p->SetName(object_name_);
    src_object_p->info().user = zgw_user_->user_info();
    src_object_p->info().mtime = now;
    etag = src_object_p->info().etag;
    s = store_->AddObject(*src_object_p);
  } else {
    etag.assign("\"" + slash::md5(req_->content) + "\"");
    libzgw::ZgwObjectInfo ob_info(now, etag, req_->content.size(), libzgw::kStandard,
                                  zgw_user_->user_info());
    libzgw::ZgwObject object(bucket_name_, object_name_, req_->content, ob_info);
    s = store_->AddObject(object);
  }
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Put object data failed: " << s.ToString();
    return;
  }
  DLOG(INFO) << "PutObject: " << req_->path << " confirm add to zp success";

  // Put object to list meta
  objects_name_->Insert(object_name_);

  DLOG(INFO) << "PutObject: " << req_->path << " confirm add to namelist success";

  if (is_copy_op) {
    resp_->SetBody(xml::CopyObjectResultXml(now, etag));
  }
  resp_->SetHeaders("ETag", etag);
  resp_->SetStatusCode(200);
}

void ZgwConn::ListObjectHandle() {
  DLOG(INFO) << "ListObjects: " << bucket_name_;

  // Check whether bucket existed in namelist meta
  if (!buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    return;
  }
  DLOG(INFO) << "ListObjects: " << req_->path << " confirm bucket exist";

  // Get objects meta from zp
  Status s;
  std::vector<libzgw::ZgwObject> objects;
  {
    std::lock_guard<std::mutex> lock(objects_name_->list_lock);
    for (auto &name : objects_name_->name_list) {
      if (name.find_first_of("__") == 0) {
        continue;
      }
      libzgw::ZgwObject object(bucket_name_, name);
      s = store_->GetObject(&object, false);
      if (!s.ok()) {
        if (s.IsNotFound()) {
          continue;
        }
        resp_->SetStatusCode(500);
        LOG(ERROR) << "ListObjects failed: " << s.ToString();
        return;
      }
      objects.push_back(object);
    }
  }
  DLOG(INFO) << "ListObjects: " << req_->path << " confirm get objects' meta from zp success";

  // Success Http response
  std::map<std::string, std::string> args{
    {"Name", bucket_name_},
    {"MaxKeys", "1000"},
    {"IsTruncated", "false"},
  };
  // args["Name"] = bucket_name_;
  // args["MaxKeys"] = "1000";
  // args["IsTruncated"] = "false";
  resp_->SetBody(xml::ListObjectsXml(objects, args));
  resp_->SetStatusCode(200);
}

void ZgwConn::DelBucketHandle() {
  DLOG(INFO) << "DeleteBucket: " << bucket_name_;
  // Check whether bucket existed in namelist meta
  if (!buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    return;
  }
  DLOG(INFO) << "DeleteBucket: " << req_->path << " confirm bucket exist";
  // Need not check return value

  if (objects_name_ != NULL && !objects_name_->IsEmpty()) {
    resp_->SetStatusCode(409);
    resp_->SetBody(xml::ErrorXml(xml::BucketNotEmpty, bucket_name_));
    LOG(ERROR) << "DeleteBucket: BucketNotEmpty";
    return;
  }

  Status s = store_->DelBucket(bucket_name_);
  if (s.ok()) {
    buckets_name_->Delete(bucket_name_);
    resp_->SetStatusCode(204);
  } else if (s.IsIOError()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Delete bucket failed: " << s.ToString();
  }

  DLOG(INFO) << "DelBucket: " << req_->path << " confirm delete from namelist success";
}

void ZgwConn::PutBucketHandle() {
  DLOG(INFO) << "CreateBucket: " << bucket_name_;

  // Check whether bucket existed in namelist meta
  if (buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(409);
    resp_->SetBody(xml::ErrorXml(xml::BucketAlreadyOwnedByYou, ""));
    return;
  }

  // Check whether belong to other user
  std::set<libzgw::ZgwUser *> user_list; // name : keys
  Status s = store_->ListUsers(&user_list);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Create bucket failed: " << s.ToString();
  }
  libzgw::NameList *tmp_bk_list;
  bool already_exist = false;
  for (auto user : user_list) {
    auto access_key = user->access_key();
    s = g_zgw_server->buckets_list()->Ref(store_, access_key, &tmp_bk_list);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "Create bucket failed: " << s.ToString();
      return;
    }
    if (tmp_bk_list->IsExist(bucket_name_)) {
      already_exist = true;
    }
    s = g_zgw_server->buckets_list()->Unref(store_, access_key);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "Create bucket failed: " << s.ToString();
      return;
    }
    if (already_exist) {
      resp_->SetStatusCode(409);
      resp_->SetBody(xml::ErrorXml(xml::BucketAlreadyExists, ""));
      return;
    }
  }

  DLOG(INFO) << "ListObjects: " << req_->path << " confirm bucket not exist";

  // Create bucket in zp
  s = store_->AddBucket(bucket_name_, zgw_user_->user_info());
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Create bucket failed: " << s.ToString();
    return;
  }
  DLOG(INFO) << "PutBucket: " << req_->path << " confirm add bucket to zp success";

  // Create list meta info
  buckets_name_->Insert(bucket_name_);

  DLOG(INFO) << "PutBucket: " << req_->path << " confirm add bucket to namelist success";

  // Success
  resp_->SetStatusCode(200);
}

void ZgwConn::ListBucketHandle() {
  DLOG(INFO) << "ListBuckets: ";
  // Find object list meta

  // Load bucket info from zp
  Status s;
  std::vector<libzgw::ZgwBucket> buckets;
  {
    std::lock_guard<std::mutex> lock(buckets_name_->list_lock);
    for (auto &name : buckets_name_->name_list) {
      libzgw::ZgwBucket bucket(name);
      s = store_->GetBucket(&bucket);
      if (!s.ok()) {
        if (s.IsNotFound()) {
          continue;
        }
        resp_->SetStatusCode(500);
        LOG(ERROR) << "ListBuckets failed: " << s.ToString();
        return;
      }
      buckets.push_back(bucket);
    }
  }

  // Zeppelin success, then build http body

  const libzgw::ZgwUserInfo &info = zgw_user_->user_info();
  resp_->SetStatusCode(200);
  resp_->SetBody(xml::ListBucketXml(info, buckets));
}
