#include <sys/time.h>

#include "zgw_conn.h"
#include "zgw_server.h"
#include "zgw_auth.h"
#include "zgw_xmlbuilder.h"
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

std::string ZgwConn::GetAccessKey(const pink::HttpRequest* req) {
  std::string auth_str;
  auto iter = req->headers.find("authorization");
  if (iter != req->headers.end())
    auth_str.assign(iter->second);
  else return "";
  size_t pos = auth_str.find("Credential");
  if (pos == std::string::npos)
    return "";
  size_t slash_pos = auth_str.find('/');
  // e.g. auth_str: "...Credential=f3oiCCuyE7v3dOZgeEBsa/20170225/us..."
  return auth_str.substr(pos + 11, slash_pos - pos - 11);
}

void ZgwConn::DealMessage(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  // DumpHttpRequest(req);

  // Get bucket name and object name
  std::string bucket_name;
  std::string object_name;
  const std::string &path = req->path;
  if (path[0] != '/') {
    resp->SetStatusCode(500);
    return;
  }
  size_t pos = path.find('/', 1);
  if (pos == std::string::npos) {
    bucket_name.assign(path.substr(1));
    object_name.clear();
  } else {
    bucket_name.assign(path.substr(1, pos - 1));
    object_name.assign(path.substr(pos + 1));
  }

  // Users operation, without authorization for now
  if (req->method == "GET" &&
      bucket_name == "admin_list_users") {
    ListUsersHandle(resp);
    return;
  } else if (req->method == "PUT" &&
             bucket_name == "admin_put_user") {
    std::string access_key;
    std::string secret_key;
    Status s = store_->AddUser(object_name, &access_key, &secret_key);
    if (!s.ok()) {
      resp->SetStatusCode(500);
      resp->SetBody(s.ToString());
    } else {
      resp->SetStatusCode(200);
      resp->SetBody(access_key + "\r\n" + secret_key);
    }
    return;
  }
  
  // Get access key
  access_key_ = GetAccessKey(req);
  // Authorize access key
  libzgw::ZgwUser *user;
  Status s = store_->GetUser(access_key_, &user);
  ZgwAuth zgw_auth;
  if (!s.ok()) {
    resp->SetStatusCode(403);
    resp->SetBody(XmlParser(InvalidAccessKeyId, ""));
    return;
  }

  // TODO (gaodq) disable request authorization
  // Authorize request
  // if (!zgw_auth.Auth(req, user->secret_key(access_key_))) {
  //   resp->SetStatusCode(403);
  //   resp->SetBody(XmlParser(SignatureDoesNotMatch, ""));
  //   return;
  // }

  if (req->method == "GET") {
    if (bucket_name.empty()) {
      ListBucketHandle(req, resp);
    } else if (!bucket_name.empty() && object_name.empty()) {
      ListObjectHandle(req, bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()){
      GetObjectHandle(req, bucket_name, object_name, resp);
    }
  } else if (req->method == "PUT") {
    if (!bucket_name.empty() && object_name.empty()) {
      PutBucketHandle(req, bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      // if (req->headers.find("Expect") != req->headers.end()) {
      //   resp->SetStatusCode(100);
      // }
      PutObjectHandle(req, bucket_name, object_name, resp);
    }
  } else if (req->method == "DELETE") {
    if (!bucket_name.empty() && object_name.empty()) {
      DelBucketHandle(req, bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      DelObjectHandle(req, bucket_name, object_name, resp);
    }
  } else if (req->method == "HEAD") {
    if (!bucket_name.empty() && object_name.empty()) {
      // * Head Bucket
      resp->SetStatusCode(200);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      // * Head Object
      resp->SetStatusCode(200);
    }
  } else {
    // Unknow request
    resp->SetStatusCode(400);
    LOG(ERROR) << "Unknow request";
  }
}

void ZgwConn::ListUsersHandle(pink::HttpResponse* resp) {
  std::set<libzgw::ZgwUser *> user_list; // name : keys
  Status s = store_->ListUsers(&user_list);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    resp->SetBody(s.ToString());
  } else {
    resp->SetStatusCode(200);
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
    resp->SetBody(body);
  }
}

void ZgwConn::DelObjectHandle(const pink::HttpRequest* req,
                              std::string &bucket_name, std::string &object_name,
                              pink::HttpResponse* resp) {
  LOG(INFO) << "DeleteObject: " << bucket_name << "/" << object_name;
  libzgw::NameList *buckets_name;
  Status s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    resp->SetStatusCode(404);
    resp->SetBody(XmlParser(NoSuchBucket, bucket_name));
    g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "DelObject: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(store_, access_key_);

  // Get objects namelist
  libzgw::NameList *objects_name;
  s = g_zgw_server->objects_list()->Ref(store_, bucket_name, &objects_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  // Check whether object existed in namelist meta
  if (!objects_name->IsExist(object_name)) {
    resp->SetStatusCode(204);
    g_zgw_server->objects_list()->Unref(store_, bucket_name);
    return;
  }
  LOG(INFO) << "DelObject: " << req->path << " confirm object exist";

  // Delete object
  s = store_->DelObject(bucket_name, object_name);
  if (!s.ok()) {
    if (s.IsNotFound()) {
      // But founded in list meta, continue to delete from list meta
    } else {
      resp->SetStatusCode(500);
      LOG(ERROR) << "Delete object data failed: " << s.ToString();
      g_zgw_server->objects_list()->Unref(store_, bucket_name);
      return;
    }
  }
  LOG(INFO) << "DelObject: " << req->path << " confirm delete object from zp success";

  // Delete from list meta
  objects_name->Delete(object_name);

  s = g_zgw_server->objects_list()->Unref(store_, bucket_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Delete object list meta failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "DelObject: " << req->path << " confirm delete object meta from namelist success";

  // Success
  resp->SetStatusCode(204);
}

void ZgwConn::GetObjectHandle(const pink::HttpRequest* req,
                              std::string &bucket_name,
                              std::string &object_name,
                              pink::HttpResponse* resp) {
  LOG(INFO) << "GetObjects: " << bucket_name << "/" << object_name;
  libzgw::NameList *buckets_name;
  Status s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    resp->SetStatusCode(404);
    resp->SetBody(XmlParser(NoSuchBucket, bucket_name));
    g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "GetObject: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(store_, access_key_);

  libzgw::NameList *objects_name;
  s = g_zgw_server->objects_list()->Ref(store_, bucket_name, &objects_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List objects name list failed: " << s.ToString();
    return;
  }

  // Check whether object existed in namelist meta
  if (!objects_name->IsExist(object_name)) {
    resp->SetStatusCode(404);
    resp->SetBody(XmlParser(NoSuchKey, object_name));
    g_zgw_server->objects_list()->Unref(store_, bucket_name);
    return;
  }
  // Need not check return value
  g_zgw_server->objects_list()->Unref(store_, bucket_name);
  LOG(INFO) << "GetObject: " << req->path << " confirm object exist";

  // Get object
  libzgw::ZgwObject object(object_name);
  s = store_->GetObject(bucket_name, object_name, &object);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Get object data failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "GetObject: " << req->path << " confirm get object from zp success";

  resp->SetStatusCode(200);
  resp->SetBody(object.content());
}

void ZgwConn::PutObjectHandle(const pink::HttpRequest *req,
                              std::string &bucket_name,
                              std::string &object_name,
                              pink::HttpResponse* resp) {
  LOG(INFO) << "PutObjcet: " << bucket_name << "/" << object_name;
  libzgw::NameList *buckets_name;
  Status s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    resp->SetStatusCode(404);
    resp->SetBody(XmlParser(NoSuchBucket, bucket_name));
    g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "GetObject: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(store_, access_key_);

  // Get objects namelist
  libzgw::NameList *objects_name;
  s = g_zgw_server->objects_list()->Ref(store_, bucket_name, &objects_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List objects name list failed: " << s.ToString();
    return;
  }

  // Put object data
  libzgw::ZgwUser *user;
  store_->GetUser(access_key_, &user);
  libzgw::ZgwObjectInfo ob_info(time(NULL),
                                slash::md5(req->content),
                                req->content.size(),
                                libzgw::kStandard,
                                user->user_info());
  s = store_->AddObject(bucket_name, object_name, ob_info, req->content);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Put object data failed: " << s.ToString();
    g_zgw_server->objects_list()->Unref(store_, bucket_name);
    return;
  }
  LOG(INFO) << "PutObject: " << req->path << " confirm add to zp success";

  // Put object to list meta
  objects_name->Insert(object_name);

  // Unref objects namelist
  s = g_zgw_server->objects_list()->Unref(store_, bucket_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Unref object namelist failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "PutObject: " << req->path << " confirm add to namelist success";

  resp->SetStatusCode(200);
}

void ZgwConn::ListObjectHandle(const pink::HttpRequest* req,
                               std::string &bucket_name,
                               pink::HttpResponse* resp) {
  LOG(INFO) << "ListObjects: " << bucket_name;
  libzgw::NameList *buckets_name;
  Status s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    resp->SetStatusCode(404);
    resp->SetBody(XmlParser(NoSuchBucket, bucket_name));
    g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(store_, access_key_);

  libzgw::NameList *objects_name;
  s = g_zgw_server->objects_list()->Ref(store_, bucket_name, &objects_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List objects name list failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm get objects' namelist success";

  // Get objects meta from zp
  std::vector<libzgw::ZgwObject> objects;
  s = store_->ListObjects(bucket_name, objects_name, &objects);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "ListBuckets Error: " << s.ToString();
    g_zgw_server->objects_list()->Unref(store_, bucket_name);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm get objects' meta from zp success";

  // Success Http response
  std::map<std::string, std::string> args;
  args["Name"] = bucket_name;
  args["MaxKeys"] = "1000";
  args["IsTruncated"] = "false";
  resp->SetStatusCode(200);
  resp->SetBody(XmlParser(objects, args));
  g_zgw_server->objects_list()->Unref(store_, bucket_name);
}

// TODO (gaodq) just delete namelist info
void ZgwConn::DelBucketHandle(const pink::HttpRequest* req,
                              std::string &bucket_name,
                              pink::HttpResponse* resp) {
  LOG(INFO) << "DeleteBucket: " << bucket_name;
  libzgw::NameList *buckets_name;
  Status s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    resp->SetStatusCode(204);
    g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm bucket exist";
  // Need not check return value

  s = store_->DelBucket(bucket_name);
  if (s.ok()) {
    buckets_name->Delete(bucket_name);
    resp->SetStatusCode(204);
  } else if (s.IsIOError()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Delete bucket failed: " << s.ToString();
  } else {
    // TODO (gaodq)
    resp->SetStatusCode(409);
    resp->SetBody(XmlParser(BucketNotEmpty, bucket_name));
    LOG(ERROR) << "Delete bucket failed: " << s.ToString();
  }

  s = g_zgw_server->buckets_list()->Unref(store_, access_key_);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Delete bucket from namelist failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "DelBucket: " << req->path << " confirm delete from namelist success";
}

void ZgwConn::PutBucketHandle(const pink::HttpRequest* req,
                              std::string &bucket_name,
                              pink::HttpResponse* resp) {
  LOG(INFO) << "CreateBucket: " << bucket_name;
  libzgw::NameList *buckets_name;
  Status s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }

  // Check whether bucket existed in namelist meta
  if (buckets_name->IsExist(bucket_name)) {
    resp->SetStatusCode(409);
    g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm bucket exist";

  // Create bucket in zp
  libzgw::ZgwUser *user;
  store_->GetUser(access_key_, &user);
  s = store_->AddBucket(bucket_name, user->user_info());
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Create bucket failed: " << s.ToString();
    s = g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "PutBucket: " << req->path << " confirm add bucket to zp success";

  // Create list meta info
  buckets_name->Insert(bucket_name);

  s = g_zgw_server->buckets_list()->Unref(store_, access_key_);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Insert bucket namelist failed: " << s.ToString();
  }
  LOG(INFO) << "PutBucket: " << req->path << " confirm add bucket to namelist success";

  // Success
  resp->SetStatusCode(200);
}

void ZgwConn::ListBucketHandle(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  LOG(INFO) << "ListBuckets: ";
  // Find object list meta
  libzgw::NameList *buckets_name;
  Status s = g_zgw_server->buckets_list()->Ref(store_, access_key_, &buckets_name);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    return;
  }
  LOG(INFO) << "ListAllBucket: " << req->path << " confirm get bucket namelist success";

  // Load bucket info from zp
  std::vector<libzgw::ZgwBucket> buckets;
  s = store_->ListBuckets(buckets_name, &buckets);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "ListBuckets Error: " << s.ToString();
    s = g_zgw_server->buckets_list()->Unref(store_, access_key_);
    return;
  }
  LOG(INFO) << "ListAllBucket: " << req->path << " confirm get bucket meta from zp success";

  // Zeppelin success, then build http body

  libzgw::ZgwUser *user = NULL;
  // Will return previously if error occured
  s = store_->GetUser(access_key_, &user);
  assert(user);
  const libzgw::ZgwUserInfo &info = user->user_info();
  resp->SetStatusCode(200);
  resp->SetBody(XmlParser(info, buckets));

  s = g_zgw_server->buckets_list()->Unref(store_, access_key_);
}
