#ifndef ZGW_XMLBUILDER_H
#define ZGW_XMLBUILDER_H

#include <string>
#include <map>

namespace xml {

enum ErrorType {
  InvalidAccessKeyId,
  SignatureDoesNotMatch,
  NoSuchBucket,
  NoSuchKey,
  BucketNotEmpty,
  NoSuchUpload,
  NotImplemented,
  BucketAlreadyOwnedByYou,
};

enum MutilUploadOp {
  InitiateMultipartUploadResult,
  CompleteMultipartUploadResult,
  ListPartsResult,
  CopyPartResult,
  CopyObjectResult,
  ListMultipartUploadsResult,
};

extern std::string ErrorXml(ErrorType etype, std::string extra_info);
extern std::string ListBucketXml(const libzgw::ZgwUserInfo &info,
                                 const std::vector<libzgw::ZgwBucket> &buckets);
extern std::string ListObjectsXml(const std::vector<libzgw::ZgwObject> &objects,
                                  std::map<std::string, std::string> &args);
extern std::string InitiateMultipartUploadResultXml(std::string &bucket_name, std::string &key,
                                                 std::string &upload_id);
extern std::string ListMultipartUploadsResultXml(const std::vector<libzgw::ZgwObject> &objects,
                                                 std::map<std::string, std::string> &args);

static std::string iso8601_time(time_t sec, suseconds_t usec = 0);

}  // namespace xml

#endif
