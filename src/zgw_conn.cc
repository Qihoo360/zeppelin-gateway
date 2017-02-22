#include <sys/time.h>

#include "zgw_conn.h"
#include "zgw_server.h"

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
  if (req->content.size() > 200) {
    LOG(INFO) << " + content: " << req->content.substr(0);
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
                 pink::WorkerThread<ZgwConn>* worker)
      : HttpConn(fd, ip_port) {
  store_ = g_zgw_server->GetStore();
}

void ZgwConn::DealMessage(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  // DumpHttpRequest(req);

  // Get bucket name and object name
  std::string bucket_name;
  std::string object_name;
  const std::string &path = req->path;
  assert(path[0] == '/');
  size_t pos = path.find('/', 1);
  if (pos == std::string::npos) {
    bucket_name.assign(path.substr(1));
    object_name.clear();
  } else {
    bucket_name.assign(path.substr(1, pos - 1));
    object_name.assign(path.substr(pos + 1));
  }

  if (req->method == "GET") {
    if (bucket_name.empty()) {
      LOG(INFO) << "ListBuckets: ";
      ListBucketHandle(resp);
    } else if (!bucket_name.empty() && object_name.empty()) {
      LOG(INFO) << "ListObjects: " << bucket_name;
      ListObjectHandle(bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()){
      LOG(INFO) << "GetObjects: " << bucket_name << "/" << object_name;
      GetObjectHandle(bucket_name, object_name, resp);
    }
  } else if (req->method == "PUT") {
    if (!bucket_name.empty() && object_name.empty()) {
      LOG(INFO) << "CreateBucket: " << bucket_name;
      PutBucketHandle(bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      // TODO gaodq test 100-continue
      if (req->headers.find("Expect") != req->headers.end()) {
        resp->SetStatusCode(100);
      } else {
        LOG(INFO) << "PutObjcet: " << bucket_name << "/" << object_name;
        PutObjectHandle(req, bucket_name, object_name, resp);
      }
    }
  } else if (req->method == "DELETE") {
    if (!bucket_name.empty() && object_name.empty()) {
      LOG(INFO) << "DeleteBucket: " << bucket_name;
      DelBucketHandle(bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      LOG(INFO) << "DeleteObject: " << bucket_name << "/" << object_name;
      DelObjectHandle(bucket_name, object_name, resp);
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
    LOG(ERROR) << "400, Unknow request";
  }
}

void ZgwConn::DelObjectHandle(std::string &bucket_name,
                              std::string &object_name,
                              pink::HttpResponse* resp) {
  slash::Status s = store_->DelObject(bucket_name, object_name);
  if (s.ok()) {
    resp->SetStatusCode(204);
  } else {
    resp->SetStatusCode(403);
    LOG(ERROR) << "Delete object failed: " << s.ToString();
  }
}

void ZgwConn::GetObjectHandle(std::string &bucket_name,
                              std::string &object_name,
                              pink::HttpResponse* resp) {
  libzgw::ZgwObject object(object_name);
  Status s = store_->GetObject(bucket_name, object_name, &object);
  if (s.ok()) {
    resp->SetStatusCode(200);
    resp->SetBody(object.content());
  } else {
    resp->SetStatusCode(204);
    LOG(ERROR) << "Get object failed: " << s.ToString();
  }
}

void ZgwConn::PutObjectHandle(const pink::HttpRequest *req,
                              std::string &bucket_name,
                              std::string &object_name,
                              pink::HttpResponse* resp) {
  libzgw::ZgwUser zgw_user(123456, "infra");
  libzgw::ZgwObjectInfo ob_info(time(NULL),
                                slash::md5(req->content),
                                req->content.size(),
                                libzgw::kStandard,
                                zgw_user);
  libzgw::ZgwObject object(object_name, req->content, ob_info);
  Status s = store_->AddObject(bucket_name, object);
  if (s.ok()) {
    resp->SetStatusCode(200);
  } else {
    resp->SetStatusCode(403);
    LOG(ERROR) << "Put object failed: " << s.ToString();
  }
}

// TODO gaodq Not Implement
void ZgwConn::ListObjectHandle(std::string &bucket_name,
                               pink::HttpResponse* resp) {
  // * List bucekts
  std::vector<libzgw::ZgwObject> objects;
  slash::Status s = store_->ListObjects(bucket_name, &objects);

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
  std::vector<libzgw::ZgwObjectInfo> infos;
  // TODO gaodq
  std::vector<std::string> cdates;
  std::vector<std::string> sizes;
  std::vector<std::string> ids;
  for (auto &object : objects) {
    infos.push_back(object.info());
    libzgw::ZgwUser &user = infos.back().user;
    //  <Contents>
    xml_node<> *content =
      doc.allocate_node(node_element, "Contents", bucket_name.c_str());
    rnode->append_node(content);
    //    <Key>
    content->append_node(doc.allocate_node(node_element, "Key",
                                           object.name().c_str()));
    //    <LastModified>
    cdates.push_back(iso8601_time(infos.back().mtime));
    content->append_node(doc.allocate_node(node_element, "LastModified", 
                                           cdates.back().c_str()));
    //    <ETag>
    content->append_node(doc.allocate_node(node_element, "ETag",
                                           infos.back().etag.c_str()));
    //    <Size>
    sizes.push_back(std::to_string(infos.back().size));
    content->append_node(doc.allocate_node(node_element, "Size",
                                           sizes.back().c_str()));
    //    <StorageClass>
    content->append_node(doc.allocate_node(node_element, "StorageClass",
                                           "STANDARD"));
    //    <Owner>
    xml_node<> *owner =
      doc.allocate_node(node_element, "Owner", user.disply_name().c_str());
    content->append_node(owner);
    //      <ID>
    ids.push_back(std::to_string(user.id()));
    owner->append_node(doc.allocate_node(node_element, "ID",
                                         ids.back().c_str()));
    //      <DisplayName>
    owner->append_node(doc.allocate_node(node_element, "DisplayName",
                                         user.disply_name().c_str()));
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);

  if (s.ok()) {
    resp->SetStatusCode(200);
    resp->SetBody(res_xml);
  } else {
    resp->SetStatusCode(204);
    LOG(ERROR) << "List object failed: " << s.ToString();
  }
}

void ZgwConn::DelBucketHandle(std::string &bucket_name,
                              pink::HttpResponse* resp) {
  slash::Status s = store_->DelBucket(bucket_name);
  if (s.ok()) {
    resp->SetStatusCode(204);
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
    error->append_node(doc.allocate_node(node_element, "BucketName", bucket_name.c_str()));
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
}

void ZgwConn::PutBucketHandle(std::string &bucket_name,
                              pink::HttpResponse* resp) {
  libzgw::ZgwBucket new_bucket(bucket_name);
  slash::Status s = store_->AddBucket(new_bucket);
  if (s.ok()) {
    resp->SetStatusCode(200);
  } else {
    resp->SetStatusCode(409);
    LOG(ERROR) << "Create bucket failed: " << s.ToString();
  }
}

std::string ZgwConn::iso8601_time(time_t sec, suseconds_t usec) {
  int milli = usec / 1000;
  char buf[128];
  strftime(buf, sizeof buf, "%FT%T", localtime(&sec));
  sprintf(buf, "%s.%03dZ", buf, milli);
  return std::string(buf);
}

void ZgwConn::ListBucketHandle(pink::HttpResponse* resp) {

  // * List bucekts
  std::vector<libzgw::ZgwBucket> buckets;
  slash::Status s = store_->ListBuckets(&buckets);

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
  xml_node<> *owner = doc.allocate_node(node_element, "Owner");
  owner->append_node(doc.allocate_node(node_element, "ID", "infra"));
  owner->append_node(doc.allocate_node(node_element, "DisplayName", "infra"));
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
                                               bucket.name().c_str()));
    cdates.push_back(iso8601_time(bucket.ctime().tv_sec, bucket.ctime().tv_usec));
    bucket_node->append_node(doc.allocate_node(node_element, "CreationDate",
                                               cdates.back().c_str()));
    buckets_node->append_node(bucket_node);
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);

  if (s.ok()) {
    resp->SetStatusCode(200);
    resp->SetBody(res_xml);
  } else {
    resp->SetStatusCode(204);
    LOG(ERROR) << "List bucket failed: " << s.ToString();
  }
}
