#ifndef ZGW_S3_REST_H
#define ZGW_S3_REST_H

#include "pink/include/http_conn.h"

#include "src/s3_cmds/zgw_s3_command.h"

class ZgwHttpHandles : public pink::HttpHandles {
 public:
  ZgwHttpHandles()
      : need_100_continue_(false),
        cmd_(nullptr) {
    cmd_table_ = new S3CmdTable;
    InitCmdTable(cmd_table_);
  }
  ~ZgwHttpHandles() {
    DestroyCmdTable(cmd_table_);
    delete cmd_table_;
  }

  virtual bool ReqHeadersHandle(const pink::HttpRequest* req) override;
  virtual void ReqBodyPartHandle(const char* data, size_t data_size) override;
  virtual void RespHeaderHandle(pink::HttpResponse* resp) override;
  virtual int RespBodyPartHandle(char* buf, size_t max_size) override;

 private:
  S3CmdTable* cmd_table_;
  S3Cmd* cmd_;
  S3Cmd* SelectS3Cmd(const pink::HttpRequest* req);

  bool need_100_continue_;
};

static ZgwHttpHandles zgw_handles;

class ZgwConnFactory : public pink::ConnFactory {
 public:
  // Deleted in HttpConn deconstructor
  virtual pink::PinkConn* NewPinkConn(int connfd,
                                      const std::string& ip_port,
                                      pink::Thread* thread) const {
    // Deleted in HttpConn's deconstructor
    ZgwHttpHandles* zgw_handles = new ZgwHttpHandles();
    return new pink::HttpConn(connfd, ip_port, thread, zgw_handles);
  }
};

#endif
