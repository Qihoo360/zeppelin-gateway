#ifndef ZGW_S3_OBJECT_H
#define ZGW_S3_OBJECT_H
#include "src/s3_cmds/zgw_s3_command.h"

#include <queue>
#include <tuple>

#include "slash/include/slash_status.h"
#include "src/zgwstore/zgw_define.h"
#include "src/zgw_utils.h"

using slash::Status;

class GetObjectCmd : public S3Cmd {
 public:
  GetObjectCmd()
      : need_partial_(false) {
    block_buffer_.resize(zgwstore::kZgwBlockSize);
  }

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void ParseBlocksFrom(const std::vector<std::string>& block_indexes);
  zgwstore::Object object_;

  uint64_t data_size_;
  bool need_partial_;
  std::string range_result_;
  //                 block_index start_bytes  size
  std::queue<std::tuple<uint64_t, uint64_t, uint64_t>> blocks_;
  std::string block_buffer_;
};

class HeadObjectCmd : public S3Cmd {

 public:
  HeadObjectCmd() {}

  virtual bool DoInitial() override;
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
  PutObjectCopyCmd() {}

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();

  std::string src_bucket_name_;
  std::string src_object_name_;
  zgwstore::Object src_object_;
  zgwstore::Object new_object_;
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
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
};

class InitMultipartUploadCmd: public S3Cmd {

 public:
  InitMultipartUploadCmd() {}

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();

  std::string upload_id_;
};

class AbortMultiUploadCmd : public S3Cmd {

 public:
  AbortMultiUploadCmd() {}

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  std::string upload_id_;
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
  zgwstore::Object new_object_part_;

  Status status_;

  MD5Ctx md5_ctx_;
  size_t block_count_;
  uint64_t block_start_;
  uint64_t block_end_;
};

class UploadPartCopyCmd : public S3Cmd {

 public:
  UploadPartCopyCmd() {}

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();

  std::string upload_id_;
  std::string part_number_;
  std::string src_bucket_name_;
  std::string src_object_name_;
  zgwstore::Object src_object_;
  zgwstore::Object new_object_;
};

class UploadPartCopyPartialCmd : public S3Cmd {

 public:
  UploadPartCopyPartialCmd() {}

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();
  void ParseBlocksFrom(const std::vector<std::string>& block_indexes);

  std::string src_bucket_name_;
  std::string src_object_name_;
  std::string upload_id_;
  std::string part_number_;
  uint64_t data_size_;
  bool need_copy_data_;
  std::string src_data_block_;
  zgwstore::Object src_object_;
  zgwstore::Object new_object_;
  std::queue<std::tuple<uint64_t, uint64_t, uint64_t>> blocks_;

  Status status_;

  MD5Ctx md5_ctx_;
};

class ListPartsCmd: public S3Cmd {

 public:
  ListPartsCmd() {}

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  struct ObjectsComparator {
    bool operator()(const zgwstore::Object& a, const zgwstore::Object& b) {
      return a.object_name < b.object_name;
    }
  };

  void GenerateRespXml();

  std::string upload_id_;
  int max_parts_;
  std::string part_num_marker_;
  std::set<zgwstore::Object, ObjectsComparator> all_candicate_parts_;
};

class CompleteMultiUploadCmd : public S3Cmd {

 public:
  CompleteMultiUploadCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  bool ParseReqXml();
  void GenerateRespXml();

  MD5Ctx md5_ctx_;
  zgwstore::Object new_object_;
  std::string upload_id_;
  std::vector<std::pair<std::string, std::string>> received_parts_info_;
};

#endif
