#include "slash_string.h"
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

  const std::string &path = req->path;
  std::vector<std::string> elems;
  slash::StringSplit(path, '/', elems);
  libzgw::ZgwStore *store = g_zgw_server->GetStore();
  using namespace rapidxml;
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string("xml version='1.0' encoding='utf-8'"));
  doc.append_node(rot);

  if (req->method == "GET") {
    if (elems.size() == 0) {
      // * List bucekts
      std::vector<libzgw::ZgwBucket> buckets;
      slash::Status s = store->ListBucket(&buckets);
  
      //    * Build Response
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

      resp->SetStatusCode(200);
      resp->SetBody(res_xml);
    } else if (elems.size() < 2) {
      // * List object
      //    * Build Response
    } else {
      // * Get object
      //    * Build Response
    }
  
  } else if (req->method == "PUT") {
  } else if (req->method == "DELETE") {
  } else if (req->method == "HEAD") {
  } else {
    // Unknow request
    resp->SetStatusCode(400);
  }
}

