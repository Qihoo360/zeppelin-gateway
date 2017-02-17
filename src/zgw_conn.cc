#include "slash_string.h"
#include "zgw_conn.h"
#include "zgw_server.h"

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

  if (req->method == "GET") {
    if (elems.size() == 0) {
      // * List bucekts
      std::vector<libzgw::ZgwBucket> buckets;
      slash::Status s = store->ListBucket(&buckets);
  
      //    * Build Response
      resp->SetStatusCode(200);
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

