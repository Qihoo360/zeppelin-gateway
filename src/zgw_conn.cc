#include "zgw_conn.h"


ZgwConn::ZgwConn(const int fd, const std::string &ip_port,
      pink::WorkerThread<ZgwConn>* worker) :
    HttpConn(fd, ip_port) {
  }


// TODO wk whether distinguish get and post in http conn or 
// DealMessage
virtual bool ZgwConn::HandleGet(pink::HttpRequest* req,
    pink::HttpResponse* res) {
  std::cout << "handle get"<< std::endl;
  for (auto item : req->headers) {
    std::cout << "header: " << item.second << std::endl;
  }
  std::cout << "conent: " << req->content << std::endl;
  // TODO wk res->SetContent(code, content)
  res->content.append("HTTP/1.1 200 OK\r\n");
  if (req->path == std::string("/country")) {
    res->content.append("Content-Length:7");
    res->content.append("\r\n\r\n");
    res->content.append("china\r\n");
    return true;
  }
  return true;
}

virtual bool ZgwConn::HandlePost(pink::HttpRequest* req,
    pink::HttpResponse* res) {
  if (req->path == std::string("/name")) {
    res->content.append("receive post: \"name="+
        req->query_params["name"] + "\"\n");
    return true;
  }
  res->content.append("this page does not exist");
  return true;
}
