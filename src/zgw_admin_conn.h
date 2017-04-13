#ifndef ZGW_ADMIN_CONN_H
#define ZGW_ADMIN_CONN_H

#include "pink/include/http_conn.h"
#include "libzgw/zgw_store.h"

class AdminConn : public pink::HttpConn {
 public:
  AdminConn(const int fd, const std::string &ip_port,
            pink::Thread* worker);
 private:
  virtual void DealMessage(const pink::HttpRequest* req,
                           pink::HttpResponse* res) override;

  void ListUsersHandle(pink::HttpResponse* resp);

  libzgw::ZgwStore *store_;
};

class AdminConnFactory : public pink::ConnFactory {
 public:
  virtual pink::PinkConn* NewPinkConn(int connfd,
                                      const std::string& ip_port,
                                      pink::Thread* thread) const {
    return new AdminConn(connfd, ip_port, thread);
  }
};

#endif
