#ifndef ZGW_ADMIN_CONN_H
#define ZGW_ADMIN_CONN_H

#include <utility>

#include "pink/include/http_conn.h"
#include "src/zgwstore/zgw_store.h"

static const std::string kAddUser = "admin_put_user";
static const std::string kListUsers = "admin_list_users";
static const std::string kGetStatus = "status";

class ZgwAdminHandles : public pink::HTTPHandles {
 public:
  ZgwAdminHandles() {}
  virtual ~ZgwAdminHandles() {}

  virtual bool HandleRequest(const pink::HTTPRequest* req, pink::HTTPResponse* resp) override;
  virtual void ReadBodyData(const char* data, size_t data_size) override {}
  virtual void PrepareResponse(pink::HTTPResponse* resp) override;
  virtual int WriteBodyData(char* buf, size_t max_size) override;

 private:
  void Initialize();

  std::string GetZgwStatus();
  std::pair<std::string, std::string> GenerateKeyPair();

  zgwstore::ZgwStore* store_;
  std::string command_;
  std::string params_;
  
  int http_ret_code_;
  std::string result_;
};

class ZgwAdminConnFactory : public pink::ConnFactory {
 public:
  virtual pink::PinkConn* NewPinkConn(int connfd,
                                      const std::string& ip_port,
                                      pink::Thread* thread) const {
    auto handles = std::make_shared<ZgwAdminHandles>();
    return new pink::HTTPConn(connfd, ip_port, thread, handles);
  }
};

#endif
