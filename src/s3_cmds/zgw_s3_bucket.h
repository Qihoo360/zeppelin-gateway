#ifndef ZGW_S3_BUCKET_H
#define ZGW_S3_BUCKET_H

#include "src/s3_cmds/zgw_s3_command.h"

#include "slash/include/slash_status.h"
#include "src/zgwstore/zgw_define.h"

using slash::Status;

class ListAllBucketsCmd : public S3Cmd {
 public:
  ListAllBucketsCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();

  std::vector<zgwstore::Bucket> all_buckets_;
};

class ListObjectsCmd : public S3Cmd {
 public:
  ListObjectsCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  bool SanitizeParams(std::string* invalid_param);
  void GenerateRespXml();

  // Common
  std::string delimiter_;
  std::string prefix_;
  int max_keys_;
  // list version2
  bool list_typeV2_;
  bool fetch_owner_;
  std::string continuation_token_;
  std::string start_after_;
  // list version1
  std::string marker_;

  struct ObjectsComparator {
    bool operator()(const zgwstore::Object& a, const zgwstore::Object& b) {
      return a.object_name < b.object_name;
    }
  };

  std::set<zgwstore::Object, ObjectsComparator> all_objects_;
};

class ListMultiPartUploadCmd : public S3Cmd {
 public:
  ListMultiPartUploadCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();
  bool SanitizeParams(std::string* invalid_param);

  std::string delimiter_;
  std::string prefix_;
  int max_uploads_;
  std::string key_marker_;
  std::string upload_id_marker_;

  struct BucketsComparator {
    bool operator()(const zgwstore::Bucket& a, const zgwstore::Bucket& b) {
      return a.bucket_name < b.bucket_name;
    }
  };

  std::set<zgwstore::Bucket, BucketsComparator> all_virtual_bks_;
};

class PutBucketCmd : public S3Cmd {

 public:
  PutBucketCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  zgwstore::Bucket new_bucket_;
};

class DeleteBucketCmd : public S3Cmd {

 public:
  DeleteBucketCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;
};

class DeleteMultiObjectsCmd : public S3Cmd {

 public:
  DeleteMultiObjectsCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
};

class HeadBucketCmd : public S3Cmd {

 public:
  HeadBucketCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;

 private:
  zgwstore::Bucket bucket_;
};

class GetBucketLocationCmd : public S3Cmd {

 public:
  GetBucketLocationCmd() {}

  virtual bool DoInitial(pink::HTTPResponse* resp) override;
  virtual void DoAndResponse(pink::HTTPResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();
  zgwstore::Bucket bucket_;
};

#endif
