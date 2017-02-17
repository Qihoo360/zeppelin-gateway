#include "zgw_bucket.h"

namespace libzgw {

static const std::string kBucketMetaPrefix = "__B";

ZgwBucket::ZgwBucket(const std::string& name)
  : name_(name) {
    ctime_ = time(NULL);
  }

ZgwBucket::~ZgwBucket() {
}

std::string ZgwBucket::MetaKey() const {
  return kBucketMetaPrefix + name_;
}

std::string ZgwBucket::MetaValue() const {
  char buf[256];
  sprintf(buf, "%ld", ctime_);
  return std::string(buf);
}

Status ZgwBucket::ParseMetaValue(const std::string& value) {
  if (!slash::string2l(value.data(), value.size(),
        static_cast<long*>(&ctime_))) {
    return Status::InvalidArgument("invalid ctime");
  }
  return Status::OK();
}

}
