#ifndef ZGW_XMLBUILDER_H
#define ZGW_XMLBUILDER_H

#include <string>
#include <vector>
#include <map>

#include "libzgw/zgw_store.h"

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
  MethodNotAllowed,
  InvalidArgument,
  InvalidRange,
};

extern std::string ErrorXml(ErrorType etype, const std::string extra_info);
extern std::string ListBucketXml(const libzgw::ZgwUserInfo &info,
                                 const std::vector<libzgw::ZgwBucket> &buckets);
extern std::string ListObjectsXml(const std::vector<libzgw::ZgwObject> &objects,
                                  const std::map<std::string, std::string> &args,
                                  const std::set<std::string>& commonprefixes);
extern std::string InitiateMultipartUploadResultXml(const std::string &bucket_name, const std::string &key,
                                                    const std::string &upload_id);
extern std::string ListMultipartUploadsResultXml(const std::vector<libzgw::ZgwObject> &objects,
                                                 const std::map<std::string, std::string> &args,
                                                 const std::set<std::string>& commonprefixes);
extern std::string ListPartsResultXml(const std::vector<std::pair<int, libzgw::ZgwObject>> &objects,
                                      const libzgw::ZgwUser *user, const std::map<std::string, std::string> &args);
extern std::string CompleteMultipartUploadResultXml(const std::string& bucket_name,
                                                    const std::string& object_name,
                                                    const std::string& final_etag);
extern std::string CopyObjectResultXml(timeval now, const std::string& etag);
extern std::string DeleteResultXml(const std::vector<std::string>& success_keys,
                                   const std::map<std::string, std::string>& error_keys);
extern bool ParseCompleteMultipartUploadXml(const std::string& xml,
                                            std::vector<std::pair<int, std::string>> *parts);
extern bool ParseDelMultiObjectXml(const std::string& xml, std::vector<std::string> *keys);
}  // namespace xml

#endif
