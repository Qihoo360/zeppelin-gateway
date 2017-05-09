#ifndef ZGW_COMMAND_H
#define ZGW_COMMAND_H

#include "pink/include/http_conn.h"

#include "src/zgwstore/zgw_store.h"

static const size_t kZgwBlockSize = 4194304; // 4MB

extern void InitCmdTable();
extern void DestroyCmdTable();

enum S3Commands {
  kListAllBuckets = 0,
  kDeleteBucket,
  kListObjects,
  kGetBucketLocation,
  kHeadBucket,
  kListMultiPartUpload,
  kPutBucket,
  kDeleteObject,
  kDeleteMultiObjects,
  kGetObject,
  kHeadObject,
  kPostObject,
  kPutObject,
  kPutObjectCopy,
  kInitMultipartUpload,
  kUploadPart,
  kUploadPartCopy,
  kUploadPartCopyPartial,
  kCompleteMultiUpload,
  kAbortMultiUpload,
  kListParts,
  kUnImplement,
};

enum S3ErrorType {
  InvalidAccessKeyId,
  SignatureDoesNotMatch,
  NoSuchBucket,
  NoSuchKey,
  BucketNotEmpty,
  NoSuchUpload,
  NotImplemented,
  BucketAlreadyOwnedByYou,
  BucketAlreadyExists,
  MalformedXML,
  InvalidPart,
  InvalidPartOrder,
  MethodNotAllowed,
  InvalidArgument,
  InvalidRange,
  AccessDenied,
};

class S3Cmd {
 public:
  S3Cmd() {}
  virtual ~S3Cmd() {}

  void Clear();
  virtual bool DoInitial() = 0;
  virtual void DoReceiveBody(const char* data, size_t data_size) {
  };
  virtual void DoAndResponse(pink::HttpResponse* resp) = 0;
  virtual int DoResponseBody(char* buf, size_t max_size) {
    return 0;
  };

  void SetReqHeaders(const std::map<std::string, std::string>& req_headers) {
    req_headers_ = req_headers;
  }
  void SetQueryParams(const std::map<std::string, std::string>& query_params) {
    query_params_ = query_params;
  }
  void SetBucketName(const std::string& bucket_name) {
    bucket_name_ = bucket_name;
  }
  void SetObjectName(const std::string& object_name) {
    object_name_ = object_name;
  }
  void SetStorePtr(zgwstore::ZgwStore* store) {
    store_ = store;
  }

 protected:
  virtual void ParseRequestXml() {}
  virtual void GenerateRespXml() {}

  std::string bucket_name_;
  std::string object_name_;
  std::map<std::string, std::string> req_headers_;
  std::map<std::string, std::string> query_params_;
  std::string http_request_xml_;

  int http_ret_code_;
  std::string http_response_xml_;

  zgwstore::ZgwStore* store_;

 private:
  // No copying allowed
  S3Cmd(const S3Cmd&);
  S3Cmd& operator=(const S3Cmd&);
};

extern std::map<S3Commands, S3Cmd*> g_cmd_table;

#endif
