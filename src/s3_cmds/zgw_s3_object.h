#ifndef ZGW_S3_OBJECT_H
#define ZGW_S3_OBJECT_H
#include "src/s3_cmds/zgw_s3_command.h"

#include <queue>

#include "slash/include/slash_status.h"
#include "src/zgwstore/zgw_define.h"
#include "src/zgw_util.h"

using slash::Status;

class GetObjectCmd : public S3Cmd {
 public:
  GetObjectCmd()
      : start_bytes_(0),
        need_partial_(false),
        block_buf_pos_(zgwstore::kZgwBlockSize) {
    block_buffer_.resize(zgwstore::kZgwBlockSize);
  }

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override {}
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void ParseBlocksFrom(const std::string& blocks_str,
                       const std::string& range);
  zgwstore::Object object_;

  uint64_t start_bytes_;
  uint64_t data_size_;
  bool need_partial_;
  std::string range_result_;
  std::queue<uint64_t> blocks_;
  size_t block_buf_pos_;
  std::string block_buffer_;
};

class HeadObjectCmd : public S3Cmd {

 public:
  HeadObjectCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override {}
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  zgwstore::Object object_;
};

class PostObjectCmd : public S3Cmd {

 public:
  PostObjectCmd()
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

class PutObjectCopyCmd : public S3Cmd {

 public:
  PutObjectCopyCmd()
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

class DeleteObjectCmd : public S3Cmd {

 public:
  DeleteObjectCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
};

class InitMultipartUploadCmd: public S3Cmd {

 public:
  InitMultipartUploadCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
};

class AbortMultiUploadCmd : public S3Cmd {

 public:
  AbortMultiUploadCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
};

class UploadPartCmd : public S3Cmd {

 public:
  UploadPartCmd()
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

class UploadPartCopyCmd : public S3Cmd {

 public:
  UploadPartCopyCmd()
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

class ListPartsCmd: public S3Cmd {

 public:
  ListPartsCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
};

class CompleteMultiUploadCmd : public S3Cmd {

 public:
  CompleteMultiUploadCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
};

#endif
