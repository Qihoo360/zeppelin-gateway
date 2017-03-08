#ifndef ZGW_XMLBUILDER_H
#define ZGW_XMLBUILDER_H

#include <string>
#include <map>

enum ErrorType {
  InvalidAccessKeyId,
  SignatureDoesNotMatch,
  NoSuchBucket,
  NoSuchKey,
  BucketNotEmpty,
};

extern std::string XmlParser(ErrorType etype, std::string extra_info);
extern std::string XmlParser(const libzgw::ZgwUserInfo &info,
                             const std::vector<libzgw::ZgwBucket> &buckets);
extern std::string XmlParser(const std::vector<libzgw::ZgwObject> &objects,
                             std::map<std::string, std::string> &args);

static std::string iso8601_time(time_t sec, suseconds_t usec = 0);

#endif
