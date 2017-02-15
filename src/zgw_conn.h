#ifndef ZGW_CONN_H
#define ZGW_CONN_H

#include "include/pink_thread.h"
#include "include/http_conn.h"

class ZgwConn : public pink::HttpConn {
 public:
  ZgwConn(const int fd, const std::string &ip_port,
      pink::WorkerThread<ZgwConn>* worker);
  virtual void DealMessage(pink::HttpRequest* req,
      pink::HttpResponse* res) override;
};


#endif
