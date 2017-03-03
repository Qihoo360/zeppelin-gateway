#include <sys/time.h>

#include "zgw_conn.h"
#include "zgw_server.h"
#include "libzgw/zgw_namelist.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"
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

std::string ZgwConn::iso8601_time(time_t sec, suseconds_t usec) {
  int milli = usec / 1000;
  char buf[128];
  strftime(buf, sizeof buf, "%FT%T", localtime(&sec));
  sprintf(buf, "%s.%03dZ", buf, milli);
  return std::string(buf);
}

std::string ZgwConn::GetAccessKey(const pink::HttpRequest* req) {
  std::string auth_str;
  auto iter = req->headers.find("Authorization");
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

void ZgwConn::ErrorHandle(std::string code, std::string message,
                          std::string bucket_name, std::string object_name,
                          pink::HttpResponse* resp, int resp_code) {
  // Builid failed Info
  using namespace rapidxml;
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string("xml version='1.0' encoding='utf-8'"));
  doc.append_node(rot);

  // <Error>
  xml_node<> *error =
    doc.allocate_node(node_element, "Error");
  doc.append_node(error);
  //  <Code>
  error->append_node(doc.allocate_node(node_element, "Code", code.c_str()));
  //  <Message>
  error->append_node(doc.allocate_node(node_element, "Message", message.c_str()));
  //  <BucketName> or <Key>
  if (!bucket_name.empty()) {
    error->append_node(doc.allocate_node(node_element, "BucketName", bucket_name.c_str()));
  } else if (!object_name.empty()) {
    error->append_node(doc.allocate_node(node_element, "Key", object_name.c_str()));
  }
  //  <RequestId> TODO gaodq
  error->append_node(doc.allocate_node(node_element, "RequestId",
                                       "tx00000000000000000113c-0058a43a07-7deaf-sh-bt-1"));
  //  <HostId> TODO gaodq
  error->append_node(doc.allocate_node(node_element, "HostId" "7deaf-sh-bt-1-sh"));

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);

  resp->SetStatusCode(resp_code);
  resp->SetBody(res_xml);
  LOG(ERROR) << message;
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
  
  // Get access key
  access_key_ = GetAccessKey(req);

  if (req->method == "GET") {
    // TODO (gaodq) administrator interface
    if (bucket_name == "admin_list_users") {
      ListUsersHandle(resp);
      return;
    }
    if (bucket_name.empty()) {
      LOG(INFO) << "ListBuckets: ";
      ListBucketHandle(req, resp);
    } else if (!bucket_name.empty() && object_name.empty()) {
      LOG(INFO) << "ListObjects: " << bucket_name;
      ListObjectHandle(req, bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()){
      LOG(INFO) << "GetObjects: " << bucket_name << "/" << object_name;
      GetObjectHandle(req, bucket_name, object_name, resp);
    }
  } else if (req->method == "PUT") {
    // TODO (gaodq) administrator interface
    if (bucket_name == "admin_put_user") {
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
    if (!bucket_name.empty() && object_name.empty()) {
      LOG(INFO) << "CreateBucket: " << bucket_name;
      PutBucketHandle(req, bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      // if (req->headers.find("Expect") != req->headers.end()) {
      //   resp->SetStatusCode(100);
      // }
      LOG(INFO) << "PutObjcet: " << bucket_name << "/" << object_name;
      PutObjectHandle(req, bucket_name, object_name, resp);
    }
  } else if (req->method == "DELETE") {
    if (!bucket_name.empty() && object_name.empty()) {
      LOG(INFO) << "DeleteBucket: " << bucket_name;
      // TODO (gaodq) DelBucketHandle(req, bucket_name, resp);
      resp->SetStatusCode(204);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      LOG(INFO) << "DeleteObject: " << bucket_name << "/" << object_name;
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
      body.append("access_keys and secret_keys: \r\n");
      // TODO (gaodq) should be std::map
      for (auto &akey : user->access_keys()) {
        std::string k = akey + "\r\n";
        body.append(k);
      }
      for (auto &skey : user->secret_keys()) {
        std::string k = skey + "\r\n";
        body.append(k);
      }
      body.append("\r\n");
    }
    resp->SetBody(body);
  }
}

Status ZgwConn::BucketListRefHandle(libzgw::NameList **buckets_name,
                                    pink::HttpResponse *resp) {
  Status s = g_zgw_server->buckets_list()->
    Ref(access_key_, store_, access_key_, buckets_name);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else {
      resp->SetStatusCode(500);
      LOG(ERROR) << "List buckets name list failed: " << s.ToString();
    }
    return s;
  }

  return Status::OK();
}

Status ZgwConn::ObjectListRefHandle(std::string &bucket_name,
                                    libzgw::NameList **objects_name,
                                    pink::HttpResponse *resp) {
  Status s = g_zgw_server->objects_list()->
    Ref(access_key_, store_, bucket_name, objects_name);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else {
      resp->SetStatusCode(500);
      LOG(ERROR) << "List objects name list failed: " << s.ToString();
    }
    return s;
  }

  return Status::OK();
}

void ZgwConn::DelObjectHandle(const pink::HttpRequest* req,
                              std::string &bucket_name,
                              std::string &object_name,
                              pink::HttpResponse* resp) {
  libzgw::NameList *buckets_name;
  Status s = BucketListRefHandle(&buckets_name, resp);
  if (!s.ok()) {
    // Response has built in BucketListRefHandle(...)
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    ErrorHandle("NoSuchBucket", "The specified bucket does not exist.",
                bucket_name, "", resp, 404);
    g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "DelObject: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);

  // Get objects namelist
  libzgw::NameList *objects_name;
  s = ObjectListRefHandle(bucket_name, &objects_name, resp);
  if (!s.ok()) {
    // Response has built in ObjectListRefHandle(...)
    return;
  }

  // Check whether object existed in namelist meta
  if (!objects_name->IsExist(object_name)) {
    ErrorHandle("NoSuchKey", "The specified bucket does not exist.",
                "", object_name, resp, 404);
    g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
    return;
  }
  LOG(INFO) << "DelObject: " << req->path << " confirm object exist";

  // Delete object
  s = store_->DelObject(access_key_, bucket_name, object_name);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else if (s.IsNotFound()) {
      // But founded in list meta, continue to delete from list meta
    } else {
      resp->SetStatusCode(500);
      LOG(ERROR) << "Delete object data failed: " << s.ToString();
      g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
      return;
    }
  }
  LOG(INFO) << "DelObject: " << req->path << " confirm delete object from zp success";

  // Delete from list meta
  objects_name->Delete(object_name);

  s = g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
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
  libzgw::NameList *buckets_name;
  Status s = BucketListRefHandle(&buckets_name, resp);
  if (!s.ok()) {
    // Response has built in BucketListRefHandle(...)
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    ErrorHandle("NoSuchBucket", "The specified bucket does not exist.",
                bucket_name, "", resp, 404);
    g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "GetObject: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);

  libzgw::NameList *objects_name;
  s = ObjectListRefHandle(bucket_name, &objects_name, resp);
  if (!s.ok()) {
    // Response has built in ObjectListRefHandle(...)
    return;
  }

  // Check whether object existed in namelist meta
  if (!objects_name->IsExist(object_name)) {
    ErrorHandle("NoSuchKey", "The specified bucket does not exist.",
                "", object_name, resp, 404);
    g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
    return;
  }
  // Need not check return value
  g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
  LOG(INFO) << "GetObject: " << req->path << " confirm object exist";

  // Get object
  libzgw::ZgwObject object(object_name);
  s = store_->GetObject(access_key_, bucket_name, object_name, &object);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else {
      resp->SetStatusCode(500);
      LOG(ERROR) << "Get object data failed: " << s.ToString();
    }
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
  libzgw::NameList *buckets_name;
  Status s = BucketListRefHandle(&buckets_name, resp);
  if (!s.ok()) {
    // Response has built in BucketListRefHandle(...)
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    ErrorHandle("NoSuchBucket", "The specified bucket does not exist.",
                bucket_name, "", resp, 404);
    g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "GetObject: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);

  // Get objects namelist
  libzgw::NameList *objects_name;
  s = ObjectListRefHandle(bucket_name, &objects_name, resp);
  if (!s.ok()) {
    // Response has built in ObjectListRefHandle(...)
    return;
  }

  // Put object data
  libzgw::ZgwObjectInfo ob_info(time(NULL),
                                slash::md5(req->content),
                                req->content.size(),
                                libzgw::kStandard);
  s = store_->AddObject(access_key_, bucket_name, object_name,
                               ob_info, req->content);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else if (s.IsIOError()) {
      resp->SetStatusCode(500);
      LOG(ERROR) << "Put object data failed: " << s.ToString();
    } else {
      resp->SetStatusCode(403);
      LOG(ERROR) << "Put object data failed: " << s.ToString();
    }
    g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
    return;
  }
  LOG(INFO) << "PutObject: " << req->path << " confirm add to zp success";

  // Put object to list meta
  objects_name->Insert(object_name);

  // Unref objects namelist
  s = g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
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
  libzgw::NameList *buckets_name;
  Status s = BucketListRefHandle(&buckets_name, resp);
  if (!s.ok()) {
    // Response has built in BucketListRefHandle(...)
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    ErrorHandle("NoSuchBucket", "The specified bucket does not exist.",
                bucket_name, "", resp, 404);
    g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm bucket exist";
  // Need not check return value
  g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);

  libzgw::NameList *objects_name;
  s = ObjectListRefHandle(bucket_name, &objects_name, resp);
  if (!s.ok()) {
    // Response has built in ObjectListRefHandle(...)
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm get objects' namelist success";

  // Get objects meta from zp
  std::vector<libzgw::ZgwObject> objects;
  s = store_->ListObjects(access_key_, bucket_name, objects_name, &objects);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else {
      resp->SetStatusCode(500);
      LOG(ERROR) << "ListBuckets Error: " << s.ToString();
    }
    g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << "get objects' meta from zp success";

  // Zeppelin success, then build body

  //    * Build xml Response
  // <root>
  using namespace rapidxml;
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string("xml version='1.0' encoding='utf-8'"));
  doc.append_node(rot);
  // <ListBucketResult>
  xml_node<> *rnode =
    doc.allocate_node(node_element, "ListBucketResult");
  xml_attribute<> *attr =
    doc.allocate_attribute("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");
  rnode->append_attribute(attr);
  doc.append_node(rnode);
  //    <Name>
  rnode->append_node(doc.allocate_node(node_element, "Name", bucket_name.c_str()));
  //    <Prefix>
  rnode->append_node(doc.allocate_node(node_element, "Prefix", ""));
  //    <Marker>
  rnode->append_node(doc.allocate_node(node_element, "Marker", ""));
  //    <MaxKeys>
  rnode->append_node(doc.allocate_node(node_element, "MaxKeys", "1000"));
  //    <IsTruncated>
  rnode->append_node(doc.allocate_node(node_element, "IsTruncated", "false"));

  // Save for xml parser
  std::vector<std::string> cdates;
  std::vector<std::string> sizes;
  for (auto &object : objects) {
    const libzgw::ZgwObjectInfo &info = object.info();
    const libzgw::ZgwUserInfo &user = info.user;
    //  <Contents>
    xml_node<> *content =
      doc.allocate_node(node_element, "Contents", bucket_name.data());
    rnode->append_node(content);
    //    <Key>
    content->append_node(doc.allocate_node(node_element, "Key",
                                           object.name().data()));
    //    <LastModified>
    cdates.push_back(iso8601_time(info.mtime));
    content->append_node(doc.allocate_node(node_element, "LastModified", 
                                           cdates.back().data()));
    //    <ETag>
    content->append_node(doc.allocate_node(node_element, "ETag",
                                           info.etag.data()));
    //    <Size>
    sizes.push_back(std::to_string(info.size));
    content->append_node(doc.allocate_node(node_element, "Size",
                                           sizes.back().data()));
    //    <StorageClass>
    content->append_node(doc.allocate_node(node_element, "StorageClass",
                                           "STANDARD"));
    //    <Owner>
    xml_node<> *owner =
      doc.allocate_node(node_element, "Owner");
    content->append_node(owner);
    //      <ID>
    owner->append_node(doc.allocate_node(node_element, "ID",
                                         user.user_id.data()));
    //      <DisplayName>
    owner->append_node(doc.allocate_node(node_element, "DisplayName",
                                         user.disply_name.data()));
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);

  // Http response
  resp->SetStatusCode(200);
  resp->SetBody(res_xml);
  g_zgw_server->objects_list()->Unref(access_key_, store_, bucket_name);
}

// TODO (gaodq) Unsupport
void ZgwConn::DelBucketHandle(const pink::HttpRequest* req,
                              std::string &bucket_name,
                              pink::HttpResponse* resp) {
  libzgw::NameList *buckets_name;
  Status s = BucketListRefHandle(&buckets_name, resp);
  if (!s.ok()) {
    // Response has built in BucketListRefHandle(...)
    return;
  }

  // Check whether bucket existed in namelist meta
  if (!buckets_name->IsExist(bucket_name)) {
    ErrorHandle("NoSuchBucket", "The specified bucket does not exist.",
                bucket_name, "", resp, 404);
    g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm bucket exist";
  // Need not check return value

  s = store_->DelBucket(access_key_, bucket_name);
  if (s.ok()) {
    buckets_name->Delete(bucket_name);
    resp->SetStatusCode(204);
  } else if (s.IsAuthFailed()) {
    ErrorHandle("InvalidAccessKeyId",
                "The access key Id you provided does not exist in our records.",
                "", "",
                resp, 403);
  } else if (s.IsIOError()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Delete bucket failed: " << s.ToString();
  } else {
    //    * Build xml Response
    // <root>
    using namespace rapidxml;
    xml_document<> doc;
    xml_node<> *rot =
      doc.allocate_node(node_pi, doc.allocate_string("xml version='1.0' encoding='utf-8'"));
    doc.append_node(rot);

    // <Error>
    xml_node<> *error =
      doc.allocate_node(node_element, "Error");
    doc.append_node(error);
    //  <Code>
    error->append_node(doc.allocate_node(node_element, "Code", "BucketNotEmpty"));
    //  <BucketName>
    error->append_node(doc.allocate_node(node_element, "BucketName", bucket_name.data()));
    //  <RequestId> TODO gaodq
    error->append_node(doc.allocate_node(node_element, "RequestId",
                                         "tx00000000000000000113c-0058a43a07-7deaf-sh-bt-1"));
    //  <HostId> TODO gaodq
    error->append_node(doc.allocate_node(node_element, "HostId" "7deaf-sh-bt-1-sh"));

    std::string res_xml;
    print(std::back_inserter(res_xml), doc, 0);

    resp->SetStatusCode(409);
    resp->SetBody(res_xml);
    LOG(ERROR) << "Delete bucket failed: " << s.ToString();
  }
  s = g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
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
  libzgw::NameList *buckets_name;
  Status s = BucketListRefHandle(&buckets_name, resp);
  if (!s.ok()) {
    // Response has built in BucketListRefHandle(...)
    return;
  }

  // Check whether bucket existed in namelist meta
  if (buckets_name->IsExist(bucket_name)) {
    resp->SetStatusCode(409);
    resp->SetBody("");
    g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "ListObjects: " << req->path << " confirm bucket exist";

  // Create bucket in zp
  s = store_->AddBucket(access_key_, bucket_name);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else if (s.IsIOError()) {
      resp->SetStatusCode(500);
      LOG(ERROR) << "Create bucket failed: " << s.ToString();
    } else {
      resp->SetStatusCode(409);
      LOG(ERROR) << "Create bucket failed: " << s.ToString();
    }
    s = g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "PutBucket: " << req->path << " confirm add bucket to zp success";

  // Create list meta info
  buckets_name->Insert(bucket_name);

  s = g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    LOG(ERROR) << "Insert bucket namelist failed: " << s.ToString();
  }
  LOG(INFO) << "PutBucket: " << req->path << " confirm add bucket to namelist success";

  // Success
  resp->SetStatusCode(200);
}

void ZgwConn::ListBucketHandle(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  // Find object list meta
  libzgw::NameList *buckets_name;
  Status s = BucketListRefHandle(&buckets_name, resp);
  if (!s.ok()) {
    // Response has built in BucketListRefHandle(...)
    return;
  }
  LOG(INFO) << "ListAllBucket: " << req->path << " confirm get bucket namelist success";

  // Load bucket info from zp
  std::vector<libzgw::ZgwBucket> buckets;
  s = store_->ListBuckets(access_key_, buckets_name, &buckets);
  if (!s.ok()) {
    if (s.IsAuthFailed()) {
      ErrorHandle("InvalidAccessKeyId",
                  "The access key Id you provided does not exist in our records.",
                  "", "",
                  resp, 403);
    } else {
      resp->SetStatusCode(500);
      LOG(ERROR) << "ListBuckets Error: " << s.ToString();
    }
    s = g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
    return;
  }
  LOG(INFO) << "ListAllBucket: " << req->path << " confirm get bucket meta from zp success";

  // Zeppelin success, then build http body

  libzgw::ZgwUser *user = NULL;
  // Will return previously if error occured
  s = store_->GetUser(access_key_, &user);
  assert(user);

  //    * Build xml Response
  // <root>
  using namespace rapidxml;
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string("xml version='1.0' encoding='utf-8'"));
  doc.append_node(rot);
  // <ListAllMyBucketsResult>
  xml_node<> *rnode =
    doc.allocate_node(node_element, "ListAllMyBucketsResult");
  xml_attribute<> *attr =
    doc.allocate_attribute("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  // <Owner>
  const libzgw::ZgwUserInfo &info = user->user_info();
  xml_node<> *owner = doc.allocate_node(node_element, "Owner");
  owner->append_node(doc.allocate_node(node_element, "ID",
                                       info.user_id.data()));
  owner->append_node(doc.allocate_node(node_element, "DisplayName",
                                       info.disply_name.data()));
  rnode->append_node(owner);

  // <Buckets>
  xml_node<> *buckets_node = doc.allocate_node(node_element, "Buckets");
  rnode->append_node(buckets_node);

  // Save data for xml parser
  std::vector<std::string> cdates;

  for (auto &bucket : buckets) {
    // <Bucket>
    xml_node<> *bucket_node = doc.allocate_node(node_element, "Bucket");
    bucket_node->append_node(doc.allocate_node(node_element, "Name",
                                               bucket.name().data()));
    cdates.push_back(iso8601_time(bucket.ctime().tv_sec, bucket.ctime().tv_usec));
    bucket_node->append_node(doc.allocate_node(node_element, "CreationDate",
                                               cdates.back().data()));
    buckets_node->append_node(bucket_node);
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);

  s = g_zgw_server->buckets_list()->Unref(access_key_, store_, access_key_);
  resp->SetStatusCode(200);
  resp->SetBody(res_xml);
}
