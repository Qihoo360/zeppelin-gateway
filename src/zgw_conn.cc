#include <sys/time.h>

#include "zgw_conn.h"
#include "zgw_server.h"
#include "zgw_auth.h"
#include "zgw_xml.h"
#include "libzgw/zgw_namelist.h"

#include "slash_hash.h"

extern ZgwServer* g_zgw_server;

static void DumpHttpRequest(const pink::HttpRequest* req) {
  LOG(INFO) << "handle get"<< std::endl;
  LOG(INFO) << " + method: " << req->method;
  LOG(INFO) << " + path: " << req->path;
  LOG(INFO) << " + version: " << req->version;
  if (req->content.size() > 50) {
    LOG(INFO) << " + content: " << req->content.substr(0, 50);
  } else {
    LOG(INFO) << " + content: " << req->content;
  }
  LOG(INFO) << " + headers: ";
  LOG(INFO) << "------------------------------------- ";
  for (auto h : req->headers) {
    LOG(INFO) << "   + " << h.first << "=>" << h.second;
  }
  LOG(INFO) << "------------------------------------- ";
  LOG(INFO) << "------------------------------------- ";
  LOG(INFO) << " + query_params: ";
  for (auto q : req->query_params) {
    LOG(INFO) << "   + " << q.first << "=>" << q.second;
  }
  LOG(INFO) << "------------------------------------- ";
} 

ZgwConn::ZgwConn(const int fd,
                 const std::string &ip_port,
                 pink::Thread* worker)
      : HttpConn(fd, ip_port) {
  worker_ = reinterpret_cast<ZgwWorkerThread *>(worker);
  store_ = worker_->GetStore();
}

std::string ZgwConn::GetAccessKey() {
  if (!req_->query_params["X-Amz-Credential"].empty()) {
    std::string credential_str = req_->query_params["X-Amz-Credential"];
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

void ZgwConn::DealMessage(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  // DumpHttpRequest(req);

  // Copy req and resp
  req_ = const_cast<pink::HttpRequest *>(req);
  resp_ = resp;

  // Get bucket name and object name
  const std::string path = req_->path;
  if (path[0] != '/') {
    resp_->SetStatusCode(500);
    return;
  }
  size_t pos = path.find('/', 1);
  if (pos == std::string::npos) {
    bucket_name_.assign(path.substr(1));
    object_name_.clear();
  } else {
    bucket_name_.assign(path.substr(1, pos - 1));
    object_name_.assign(path.substr(pos + 1));
  }
  if (object_name_.back() == '/') {
    object_name_.resize(object_name_.size() - 1);
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
    Status s = store_->AddUser(object_name_, &access_key, &secret_key);
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
  Status s = store_->GetUser(access_key_, &zgw_user_);
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
  auto const &info = zgw_user_->user_info();
  s = g_zgw_server->buckets_list()->Ref(store_, info.disply_name, &buckets_name_);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    s = g_zgw_server->buckets_list()->Unref(store_, info.disply_name);
    return;
  }

  if (!bucket_name_.empty() && buckets_name_->IsExist(bucket_name_)) {
    // Get objects namelist and ref
    s = g_zgw_server->objects_list()->Ref(store_, bucket_name_, &objects_name_);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "List objects name list failed: " << s.ToString();
      s = g_zgw_server->buckets_list()->Unref(store_, info.disply_name);
      return;
    }
  }

  METHOD method;
  if (req_->method == "GET") {
    method = GET;
  } else if (req_->method == "PUT") {
    method = PUT;
  } else if (req_->method == "DELETE") {
    method = DELETE;
  } else if (req_->method == "HEAD") {
    method = HEAD;
  } else if (req_->method == "POST") {
    method = POST;
  } else {
    method = UNSUPPORT;
  }

  if (bucket_name_.empty()) {
    ListBucketHandle();
  } else if (IsBucketOp()) {
    switch(method) {
      case GET:
        if (req_->query_params.find("uploads") != req_->query_params.end()) {
          ListMultiPartsUpload();
        } else {
          ListObjectHandle();
        }
        break;
      case PUT:
        PutBucketHandle();
        break;
      case DELETE:
        DelBucketHandle();
        break;
      case HEAD:
        ListObjectHandle(true);
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
      LOG(INFO) << "Object Op: " << req_->path << " confirm bucket exist";
      g_zgw_server->object_mutex()->Lock(bucket_name_ + object_name_);
      switch(method) {
        case GET:
          GetObjectHandle();
          break;
        case PUT:
          PutObjectHandle();
          break;
        case DELETE:
          if (req_->query_params.find("uploadId") != req_->query_params.end()) {
            AbortMultiUpload(req_->query_params["uploadId"]);
          } else {
            DelObjectHandle();
          }
          break;
        case HEAD:
          GetObjectHandle(true);
          break;
        case POST:
          if (req_->query_params.find("uploads") != req_->query_params.end()) {
            // Initial MultiPart Upload
            InitialMultiUpload();
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
    LOG(ERROR) << "Unknow request";
  }

  // Unref namelist
  Status s1 = Status::OK();
  s = g_zgw_server->buckets_list()->Unref(store_, info.disply_name);
  if (!bucket_name_.empty()) {
    s1 = g_zgw_server->objects_list()->Unref(store_, bucket_name_);
  }
  if (!s.ok() || !s1.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Unref namelist failed: " << s.ToString();
    return;
  }

  resp_->SetHeaders("Last-Modified", http_nowtime());
  resp_->SetHeaders("Date", http_nowtime());
}

void ZgwConn::InitialMultiUpload() {
  std::string upload_id, internal_obname;
  Status s = store_->InitMultiUpload(bucket_name_, object_name_, &upload_id, &internal_obname, zgw_user_);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    return;
  }
  LOG(INFO) << "Get upload id, and insert multiupload meta to zp";

  // Insert into namelist
  objects_name_->Insert(internal_obname);
  LOG(INFO) << "Insert into namelist: " << internal_obname;

  // Success Response
  resp_->SetBody(xml::InitiateMultipartUploadResultXml(bucket_name_,
                                                       object_name_, upload_id));
  resp_->SetStatusCode(200);
}

void ZgwConn::UploadPartHandle() {
}

void ZgwConn::CompleteMultiUpload() {
}

void ZgwConn::AbortMultiUpload(const std::string &upload_id) {
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
  LOG(INFO) << "AbortMultiUpload: " << req_->path << " confirm delete object meta from namelist success";

  // Success
  resp_->SetStatusCode(204);
}

void ZgwConn::ListParts() {
}

void ZgwConn::ListMultiPartsUpload() {
  // Check whether bucket existed in namelist meta
  if (!buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    return;
  }
  LOG(INFO) << "ListObjects: " << req_->path << " confirm bucket exist";

  std::vector<libzgw::ZgwObject> objects;
  Status s = store_->ListObjects(bucket_name_, objects_name_, &objects, true);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "ListBuckets Error: " << s.ToString();
    return;
  }
  LOG(INFO) << "ListObjects: " << req_->path << " confirm get objects' meta from zp success";

  std::map<std::string, std::string> args;
  args["Bucket"] = bucket_name_;
  std::string next_key_marker = objects.empty() ? "" : objects.back().name();
  args["NextKeyMarker"] = next_key_marker.empty() ? "" :
    next_key_marker.substr(2, next_key_marker.size() - 32 - 2);
  args["NextUploadIdMarker"] = objects.empty() ? "" : objects.back().upload_id();
  args["MaxUploads"] = "1000";
  args["IsTruncated"] = "false"; // TODO (gaodq)
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

void ZgwConn::DelObjectHandle() {
  LOG(INFO) << "DeleteObject: " << bucket_name_ << "/" << object_name_;

  // Check whether object existed in namelist meta
  if (!objects_name_->IsExist(object_name_)) {
    resp_->SetStatusCode(204);
    return;
  }
  LOG(INFO) << "DelObject: " << req_->path << " confirm object exist";

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
  LOG(INFO) << "DelObject: " << req_->path << " confirm delete object from zp success";

  // Delete from list meta
  objects_name_->Delete(object_name_);

  LOG(INFO) << "DelObject: " << req_->path << " confirm delete object meta from namelist success";

  // Success
  resp_->SetStatusCode(204);
}

void ZgwConn::GetObjectHandle(bool is_head_op) {
  LOG(INFO) << "GetObjects: " << bucket_name_ << "/" << object_name_;

  if (!objects_name_->IsExist(object_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchKey, object_name_));
    return;
  }
  LOG(INFO) << "GetObject: " << req_->path << " confirm object exist";

  if (!is_head_op) {
    // Get object
    libzgw::ZgwObject object(object_name_);
    Status s = store_->GetObject(bucket_name_, object_name_, &object);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "Get object data failed: " << s.ToString();
      return;
    }
    LOG(INFO) << "GetObject: " << req_->path << " confirm get object from zp success";

    resp_->SetBody(object.content());
  }

  resp_->SetStatusCode(200);
}

void ZgwConn::PutObjectHandle() {
  LOG(INFO) << "PutObjcet: " << bucket_name_ << "/" << object_name_;

  // Put object data
  std::string etag = "\"" + slash::md5(req_->content) + "\"";
  timeval now;
  gettimeofday(&now, NULL);
  libzgw::ZgwObjectInfo ob_info(now, etag,
                                req_->content.size(), libzgw::kStandard,
                                zgw_user_->user_info());
  Status s = store_->AddObject(bucket_name_, object_name_, ob_info, req_->content);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Put object data failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "PutObject: " << req_->path << " confirm add to zp success";

  // Put object to list meta
  objects_name_->Insert(object_name_);

  LOG(INFO) << "PutObject: " << req_->path << " confirm add to namelist success";

  resp_->SetHeaders("ETag", etag);
  resp_->SetStatusCode(200);
}

void ZgwConn::ListObjectHandle(bool is_head_op) {
  LOG(INFO) << "ListObjects: " << bucket_name_;

  // Check whether bucket existed in namelist meta
  if (!buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    return;
  }
  LOG(INFO) << "ListObjects: " << req_->path << " confirm bucket exist";

  if (!is_head_op) {
    // Get objects meta from zp
    std::vector<libzgw::ZgwObject> objects;
    Status s = store_->ListObjects(bucket_name_, objects_name_, &objects);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "ListBuckets Error: " << s.ToString();
      return;
    }
    LOG(INFO) << "ListObjects: " << req_->path << " confirm get objects' meta from zp success";

    // Success Http response
    std::map<std::string, std::string> args;
    args["Name"] = bucket_name_;
    args["MaxKeys"] = "1000";
    args["IsTruncated"] = "false";
    resp_->SetBody(xml::ListObjectsXml(objects, args));
  }
  resp_->SetStatusCode(200);
}

void ZgwConn::DelBucketHandle() {
  LOG(INFO) << "DeleteBucket: " << bucket_name_;
  // Check whether bucket existed in namelist meta
  if (!buckets_name_->IsExist(bucket_name_)) {
    resp_->SetStatusCode(404);
    resp_->SetBody(xml::ErrorXml(xml::NoSuchBucket, bucket_name_));
    return;
  }
  LOG(INFO) << "DeleteBucket: " << req_->path << " confirm bucket exist";
  // Need not check return value

  Status s;
  if (objects_name_ == NULL || !objects_name_->IsEmpty()) {
    resp_->SetStatusCode(409);
    resp_->SetBody(xml::ErrorXml(xml::BucketNotEmpty, bucket_name_));
    LOG(ERROR) << "DeleteBucket: BucketNotEmpty";
    return;
#if 0// for ceph test cases TODO (gaodq)
    {
      std::lock_guard<std::mutex> lock(objects_name_->list_lock);
      for (auto &obname : objects_name_->name_list) {
        s = store_->DelObject(bucket_name_, obname);
        if (!s.ok()) {
          resp_->SetStatusCode(500);
          LOG(ERROR) << "Delete bucket's objects failed: " << s.ToString();
          return;
        }
        objects_name_->name_list.erase(obname);
      }
      objects_name_->dirty = true;
    }
#endif
  }

  s = store_->DelBucket(bucket_name_);
  if (s.ok()) {
    buckets_name_->Delete(bucket_name_);
    resp_->SetStatusCode(204);
  } else if (s.IsIOError()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Delete bucket failed: " << s.ToString();
  }

  LOG(INFO) << "DelBucket: " << req_->path << " confirm delete from namelist success";
}

void ZgwConn::PutBucketHandle() {
  LOG(INFO) << "CreateBucket: " << bucket_name_;

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
    const auto &info = user->user_info();
    s = g_zgw_server->buckets_list()->Ref(store_, info.disply_name, &tmp_bk_list);
    if (!s.ok()) {
      resp_->SetStatusCode(500);
      LOG(ERROR) << "Create bucket failed: " << s.ToString();
      return;
    }
    if (tmp_bk_list->IsExist(bucket_name_)) {
      already_exist = true;
    }
    s = g_zgw_server->buckets_list()->Unref(store_, info.disply_name);
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

  LOG(INFO) << "ListObjects: " << req_->path << " confirm bucket not exist";

  // Create bucket in zp
  s = store_->AddBucket(bucket_name_, zgw_user_->user_info());
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "Create bucket failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "PutBucket: " << req_->path << " confirm add bucket to zp success";

  // Create list meta info
  buckets_name_->Insert(bucket_name_);

  LOG(INFO) << "PutBucket: " << req_->path << " confirm add bucket to namelist success";

  // Success
  resp_->SetStatusCode(200);
}

void ZgwConn::ListBucketHandle() {
  LOG(INFO) << "ListBuckets: ";
  // Find object list meta

  // Load bucket info from zp
  std::vector<libzgw::ZgwBucket> buckets;
  Status s = store_->ListBuckets(buckets_name_, &buckets);
  if (!s.ok()) {
    resp_->SetStatusCode(500);
    LOG(ERROR) << "ListBuckets Error: " << s.ToString();
    return;
  }
  LOG(INFO) << "ListAllBucket: " << req_->path << " confirm get bucket meta from zp success";

  // Zeppelin success, then build http body

  const libzgw::ZgwUserInfo &info = zgw_user_->user_info();
  resp_->SetStatusCode(200);
  resp_->SetBody(xml::ListBucketXml(info, buckets));
}

std::string ZgwConn::http_nowtime() {
  char buf[100] = {0};
  time_t now = time(0);
  struct tm t = *gmtime(&now);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &t);
  return std::string(buf);
}
