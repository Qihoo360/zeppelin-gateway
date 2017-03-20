#ifndef ZGW_XMLBUILDER_H
#define ZGW_XMLBUILDER_H

#include <string>
#include <vector>
#include <map>
#include "zgw_store.h"

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
  BucketAlreadyExists,
  MalformedXML,
  InvalidPart,
  InvalidPartOrder,
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
extern std::string ListPartsResultXml(const std::vector<std::pair<int, libzgw::ZgwObject>> &objects,
                                      libzgw::ZgwUser *user,
                                      std::map<std::string, std::string> &args);
extern std::string DeleteResultXml(const std::vector<std::string>& success_keys,
                                   const std::map<std::string, std::string>& error_keys);
extern bool ParseCompleteMultipartUploadXml(const std::string& xml,
                                            std::vector<std::pair<int, std::string>> *parts);
extern bool ParseDelMultiObjectXml(const std::string& xml, std::vector<std::string> *keys);
}  // namespace xml

#endif
