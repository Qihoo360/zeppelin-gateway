#include "zgw_conn.h"
#include "zgw_server.h"

extern ZgwServer* g_zgw_server;

ZgwConn::ZgwConn(const int fd, const std::string &ip_port,
      pink::WorkerThread<ZgwConn>* worker) :
    HttpConn(fd, ip_port) {
  }

void ZgwConn::DealMessage(pink::HttpRequest* req, pink::HttpResponse* res) {
  LOG(INFO) << "handle get"<< std::endl;
  LOG(INFO) << " + method: " << req->method;
  LOG(INFO) << " + path: " << req->path;
  LOG(INFO) << " + version: " << req->version;
  LOG(INFO) << " + content: " << req->version;
  LOG(INFO) << " + headers: ";
  for (auto h : req->headers) {
    LOG(INFO) << "   + " << h.first << ":" << h.second;
  }
  LOG(INFO) << " + query_params: ";
  for (auto q : req->query_params) {
    LOG(INFO) << "   + " << q.first << ":" << q.second;
  }
  res->content.append("HTTP/1.1 200 OK\r\n");
  if (req->path == std::string("/country")) {
    res->content.append("Content-Length:7");
    res->content.append("\r\n\r\n");
    res->content.append("china\r\n");
  }
}
