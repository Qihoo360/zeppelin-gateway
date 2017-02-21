#include <sys/time.h>

#include "zgw_bucket.h"
#include "include/slash_coding.h"

namespace libzgw {

static const std::string kBucketMetaPrefix = "__B";

ZgwBucket::ZgwBucket(const std::string& name)
  : name_(name) {
  gettimeofday(&ctime_, NULL);
}

ZgwBucket::~ZgwBucket() {
}

std::string ZgwBucket::MetaKey() const {
  return kBucketMetaPrefix + name_;
}

std::string ZgwBucket::MetaValue() const {
  std::string result;
  slash::PutFixed32(&result, ctime_.tv_sec);
  slash::PutFixed32(&result, ctime_.tv_usec);
  return result;
}

Status ZgwBucket::ParseMetaValue(std::string& value) {
  uint32_t tmp; 
  slash::GetFixed32(&value, &tmp);
  ctime_.tv_sec = static_cast<time_t>(tmp);
  slash::GetFixed32(&value, &tmp);
  ctime_.tv_usec = static_cast<suseconds_t>(tmp);
  return Status::OK();
}

}  // namespace libzgw
