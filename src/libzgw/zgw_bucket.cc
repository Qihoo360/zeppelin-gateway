#include <sys/time.h>

#include "zgw_bucket.h"

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
  char buf[256];
  sprintf(buf, "%ld,%ld", ctime_.tv_sec, ctime_.tv_usec);
  return std::string(buf);
}

Status ZgwBucket::ParseMetaValue(const std::string& value) {
  int ret = sscanf(value.c_str(), "%ld,%ld", &ctime_.tv_sec, &ctime_.tv_usec);
  if (ret != 2) {
    return Status::InvalidArgument("invalid ctime");
  }
  return Status::OK();
}

}  // namespace libzgw
