#include "pink/include/http_conn.h"

#include "src/s3_cmds/zgw_s3_command.h"

class ZgwHttpHandles : public pink::HttpHandles {
 public:
  ZgwHttpHandles()
      : need_100_continue_(false),
        cmd_(nullptr) {
  }
  ~ZgwHttpHandles() {}

  virtual bool ReqHeadersHandle(const pink::HttpRequest* req) override;
  virtual void ReqBodyPartHandle(const char* data, size_t data_size) override;
  virtual void RespHeaderHandle(pink::HttpResponse* resp) override;
  virtual int RespBodyPartHandle(char* buf, size_t max_size) override;

 private:
  S3Cmd* SelectS3Cmd(const pink::HttpRequest* req);

  bool need_100_continue_;
  S3Cmd* cmd_;
};

static ZgwHttpHandles zgw_handles;

class ZgwConnFactory : public pink::ConnFactory {
 public:
  // Deleted in HttpConn deconstructor
  virtual pink::PinkConn* NewPinkConn(int connfd,
                                      const std::string& ip_port,
                                      pink::Thread* thread) const {
    return new pink::HttpConn(connfd, ip_port, thread, &zgw_handles);
  }
};
