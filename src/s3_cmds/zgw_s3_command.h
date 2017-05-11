#ifndef ZGW_S3_COMMAND_H
#define ZGW_S3_COMMAND_H

#include "pink/include/http_conn.h"

#include "src/zgwstore/zgw_store.h"
#include "src/s3_cmds/zgw_s3_authv4.h"

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
  kInvalidAccessKeyId,
  kSignatureDoesNotMatch,
  kNoSuchBucket,
  kNoSuchKey,
  kBucketNotEmpty,
  kNoSuchUpload,
  kNotImplemented,
  kBucketAlreadyOwnedByYou,
  kBucketAlreadyExists,
  kMalformedXML,
  kInvalidPart,
  kInvalidPartOrder,
  kMethodNotAllowed,
  kInvalidArgument,
  kInvalidRange,
  kInvalidRequest,
  kAccessDenied,
};

class S3Cmd {
 public:
  S3Cmd()
    : store_(nullptr),
      http_ret_code_(200) {
  }
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
  void InitS3Auth(const pink::HttpRequest* req) {
    assert(store_ != nullptr);
    s3_auth_.Initialize(req, store_);
  }

 protected:
  bool TryAuth();
  virtual void GenerateRespXml() {}
  void GenerateErrorXml(S3ErrorType type, const std::string& message = "");

  // These parameters have been filled
  std::string user_name_;
  std::string bucket_name_;
  std::string object_name_;
  std::map<std::string, std::string> req_headers_;
  std::map<std::string, std::string> query_params_;
  zgwstore::ZgwStore* store_;

  S3AuthV4 s3_auth_; // Authorize in DoInitial()

  // Should clear in every request
  int http_ret_code_;
  std::string http_request_xml_;
  std::string http_response_xml_;

 private:
  // No copying allowed
  S3Cmd(const S3Cmd&);
  S3Cmd& operator=(const S3Cmd&);
};

extern std::map<S3Commands, S3Cmd*> g_cmd_table;

#endif
