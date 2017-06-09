#ifndef ZGW_ADMIN_CONN_H
#define ZGW_ADMIN_CONN_H

#include <utility>

#include "pink/include/http_conn.h"
#include "slash/include/env.h"
#include "src/zgwstore/zgw_store.h"

static const std::string kAddUser = "admin_put_user";
static const std::string kListUsers = "admin_list_users";
static const std::string kGetStatus = "status";
static const std::string kGetVersion = "version";

class ZgwAdminHandles : public pink::HTTPHandles {
 public:
  ZgwAdminHandles()
      : bk_update_time_(0) {
  }
  virtual ~ZgwAdminHandles() {}

  virtual bool HandleRequest(const pink::HTTPRequest* req) override;
  virtual void HandleBodyData(const char* data, size_t data_size) override {}
  virtual void PrepareResponse(pink::HTTPResponse* resp) override;
  virtual int WriteResponseBody(char* buf, size_t max_size) override;

 private:
  void Initialize();

  std::string GetZgwStatus(bool force = false);
  std::pair<std::string, std::string> GenerateKeyPair();
  std::string GenBucketInfo(bool force = false);
  std::string GenCommandsInfo();

  zgwstore::ZgwStore* store_;
  std::string command_;
  std::string params_;
  std::vector<zgwstore::Bucket> all_buckets_;
  uint64_t bk_update_time_;
  
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
