#ifndef ZGW_S3_BUCKET_H
#define ZGW_S3_BUCKET_H

#include "src/s3_cmds/zgw_s3_command.h"

#include "slash/include/slash_status.h"
#include "src/zgwstore/zgw_define.h"

using slash::Status;

class ListAllBucketsCmd : public S3Cmd {
 public:
  ListAllBucketsCmd() {}

  virtual bool DoInitial() override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  void GenerateRespXml();

  std::vector<zgwstore::Bucket> all_buckets_;
};

class PutBucketCmd : public S3Cmd {

 public:
  PutBucketCmd() {}

  virtual bool DoInitial() override;
  virtual void DoReceiveBody(const char* data, size_t data_size) override;
  virtual void DoAndResponse(pink::HttpResponse* resp) override;
  virtual int DoResponseBody(char* buf, size_t max_size) override;

 private:
  zgwstore::Bucket new_bucket_;
};

#endif
