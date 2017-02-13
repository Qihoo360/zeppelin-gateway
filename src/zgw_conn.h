#ifndef ZGW_CONN_H
#define ZP_BINLOG_H

#include "include/pink_thread.h"
#include "include/http_conn.h"

class ZgwConn : public pink::HttpConn {
 public:
  ZgwConn(const int fd, const std::string &ip_port,
      pink::WorkerThread<ZgwConn>* worker) :
    HttpConn(fd, ip_port) {
  }
  virtual bool HandleGet(pink::HttpRequest* req,
      pink::HttpResponse* res) override;

  virtual bool HandlePost(pink::HttpRequest* req,
      pink::HttpResponse* res) override;
};


#endif
