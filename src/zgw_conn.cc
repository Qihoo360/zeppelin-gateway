#include "zgw_conn.h"
#include "zgw_server.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidxml/rapidxml_utils.hpp"

extern ZgwServer* g_zgw_server;

static void DumpHttpRequest(const pink::HttpRequest* req) {
  LOG(INFO) << "handle get"<< std::endl;
  LOG(INFO) << " + method: " << req->method;
  LOG(INFO) << " + path: " << req->path;
  LOG(INFO) << " + version: " << req->version;
  LOG(INFO) << " + content: " << req->content;
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

ZgwConn::ZgwConn(const int fd, const std::string &ip_port,
      pink::WorkerThread<ZgwConn>* worker) :
    HttpConn(fd, ip_port) {
  }

void ZgwConn::DealMessage(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  DumpHttpRequest(req);

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
      // * List buckets
      ListBucketOperation(resp);
    } else if (!bucket_name.empty() && object_name.empty()) {
      // * List objects
      ListObjectOperation(bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()){
      // * Get object
    }
  } else if (req->method == "PUT") {
    if (!bucket_name.empty() && object_name.empty()) {
      // * Put Bucket
      CreateBucketOperation(bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      // * Put Object
    }
  } else if (req->method == "DELETE") {
    if (!bucket_name.empty() && object_name.empty()) {
      // * Delete Bucket
      DeleteBucketOperation(bucket_name, resp);
    } else if (!bucket_name.empty() && !object_name.empty()) {
      // * Delete Object
      libzgw::ZgwStore *store = g_zgw_server->GetStore();
      slash::Status s = store->DelBucket(bucket_name);
      if (s.ok()) {
        resp->SetStatusCode(204);
      } else {
        resp->SetStatusCode(403);
      }
    }
  } else if (req->method == "HEAD") {
    if (!bucket_name.empty() && object_name.empty()) {
      // * Head Bucket
    } else if (!bucket_name.empty() && !object_name.empty()) {
      // * Head Object
    }
  } else {
    // Unknow request
    resp->SetStatusCode(400);
  }
}

void ZgwConn::ListObjectOperation(std::string &bucket_name,
                                    pink::HttpResponse* resp) {
  libzgw::ZgwStore *store = g_zgw_server->GetStore();

  // * List bucekts
  std::vector<libzgw::ZgwObject> objects;
  slash::Status s = store->ListObjects(bucket_name, &objects);

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
  xml_node<> *name =
    doc.allocate_node(node_element, "Name", bucket_name.c_str());
  rnode->append_node(name);
  //    <Prefix>
  xml_node<> *prefix =
    doc.allocate_node(node_element, "Prefix", "");
  rnode->append_node(prefix);
  //    <Marker>
  xml_node<> *marker =
    doc.allocate_node(node_element, "Marker", "");
  rnode->append_node(marker);
  //    <MaxKeys>
  xml_node<> *maxkeys =
    doc.allocate_node(node_element, "MaxKeys", "1000");
  rnode->append_node(maxkeys);
  //    <IsTruncated>
  xml_node<> *is_truncated =
    doc.allocate_node(node_element, "IsTruncated", "false");
  rnode->append_node(is_truncated);
  
  for (auto &object : objects) {
    //  <Contents>
    xml_node<> *content =
      doc.allocate_node(node_element, "Contents", bucket_name.c_str());
    rnode->append_node(content);
    //    <Key>
    xml_node<> *key =
      doc.allocate_node(node_element, "Key", object.key.c_str());
    content->append_node(key);
    //    <LastModified>
    xml_node<> *lastmodified =
      doc.allocate_node(node_element, "LastModified", "last");
    content->append_node(lastmodified);
    //    <ETag>
    xml_node<> *etag =
      doc.allocate_node(node_element, "ETag", "ETag");
    content->append_node(etag);
    //    <Size>
    xml_node<> *size =
      doc.allocate_node(node_element, "Size", "size");
    content->append_node(size);
    //    <StorageClass>
    xml_node<> *storageclass =
      doc.allocate_node(node_element, "Key", "STANDARD");
    content->append_node(storageclass);
    //    <Owner>
    xml_node<> *owner =
      doc.allocate_node(node_element, "Owner", "infra");
    content->append_node(owner);
    //      <ID>
    xml_node<> *id = doc.allocate_node(node_element, "ID", "infra");
    //      <DisplayName>
    xml_node<> *disname = doc.allocate_node(node_element, "DisplayName", "infra");
    owner->append_node(id);
    owner->append_node(disname);
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);

  if (s.ok()) {
    resp->SetStatusCode(200);
    resp->SetBody(res_xml);
  } else {
    resp->SetStatusCode(400);
  }
}

void ZgwConn::DeleteBucketOperation(std::string &bucket_name,
                                    pink::HttpResponse* resp) {
  libzgw::ZgwStore *store = g_zgw_server->GetStore();
  
  slash::Status s = store->DelBucket(bucket_name);
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
    xml_node<> *code =
      doc.allocate_node(node_element, "Code", "BucketNotEmpty");
    error->append_node(code);
    //  <BucketName>
    xml_node<> *bucket_name_node =
      doc.allocate_node(node_element, "BucketName", bucket_name.c_str());
    error->append_node(bucket_name_node);
    //  <RequestId> TODO gaodq
    xml_node<> *request_id =
      doc.allocate_node(node_element, "RequestId",
                        "tx00000000000000000113c-0058a43a07-7deaf-sh-bt-1");
    error->append_node(request_id);
    //  <HostId> TODO gaodq
    xml_node<> *host_id =
      doc.allocate_node(node_element, "HostId" "7deaf-sh-bt-1-sh");
    error->append_node(host_id);

    std::string res_xml;
    print(std::back_inserter(res_xml), doc, 0);

    resp->SetStatusCode(409);
    resp->SetBody(res_xml);
  }
}

void ZgwConn::CreateBucketOperation(std::string &bucket_name,
                                    pink::HttpResponse* resp) {
  libzgw::ZgwStore *store = g_zgw_server->GetStore();

  libzgw::ZgwBucket new_bucket(bucket_name, "2017-02-14T08:35:57.833Z");
  slash::Status s = store->AddBucket(new_bucket);
  if (s.ok()) {
    resp->SetStatusCode(200);
  } else {
    resp->SetStatusCode(403);
  }
}

void ZgwConn::ListBucketOperation(pink::HttpResponse* resp) {
  libzgw::ZgwStore *store = g_zgw_server->GetStore();

  // * List bucekts
  std::vector<libzgw::ZgwBucket> buckets;
  slash::Status s = store->ListBuckets(&buckets);

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
  xml_node<> *id = doc.allocate_node(node_element, "ID", "infra");
  xml_node<> *disname = doc.allocate_node(node_element, "DisplayName", "infra");
  owner->append_node(id);
  owner->append_node(disname);
  rnode->append_node(owner);

  // <Buckets>
  xml_node<> *buckets_node = doc.allocate_node(node_element, "Buckets");
  rnode->append_node(buckets_node);

  for (auto &bucket : buckets) {
    // <Bucket>
    xml_node<> *bucket_node = doc.allocate_node(node_element, "Bucket");
    xml_node<> *name = doc.allocate_node(node_element, "Name",
                                         bucket.GetName().c_str());
    xml_node<> *date = doc.allocate_node(node_element, "CreationDate",
                                         bucket.GetDate().c_str());
    bucket_node->append_node(name);
    bucket_node->append_node(date);
    buckets_node->append_node(bucket_node);
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);

  if (s.ok()) {
    resp->SetStatusCode(200);
    resp->SetBody(res_xml);
  } else {
    resp->SetStatusCode(400);
  }
}
