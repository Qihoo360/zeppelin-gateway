#ifndef ZGW_S3_OBJECT_H
#define ZGW_S3_OBJECT_H
#include "src/s3_cmds/zgw_s3_command.h"

#include "slash/include/slash_status.h"
#include "src/zgwstore/zgw_define.h"
#include "src/zgw_util.h"

using slash::Status;

class ListObjectsCmd : public S3Cmd {
 public:
  ListObjectsCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override {}
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();

  zgwstore::Object object_;
  std::vector<zgwstore::Object> all_objects_;
};

class GetObjectCmd : public S3Cmd {
 public:
  GetObjectCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override {}
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  zgwstore::Object object_;

  size_t block_buf_pos_;
  std::string block_buffer_;
};

class PutObjectCmd : public S3Cmd {

 public:
  PutObjectCmd()
    : block_count_(0),
      block_start_(0),
      block_end_(0) {
  }

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  zgwstore::Object new_object_;

  Status status_;

  MD5Ctx md5_ctx_;
  size_t block_count_;
  uint64_t block_start_;
  uint64_t block_end_;
};

#endif
