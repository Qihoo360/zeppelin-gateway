#include <sys/time.h>
#include <assert.h>

#include "zgw_bucket.h"
#include "include/slash_coding.h"

namespace libzgw {

static const std::string kBucketMetaPrefix = "__B__";

ZgwBucket::ZgwBucket(const std::string& name)
  : name_(name),
    object_count_(0) {
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
  slash::PutFixed32(&result, objects_name_.size());

  for (auto &name : objects_name_) {
    slash::PutLengthPrefixedString(&result, name);
  }
  return result;
}

Status ZgwBucket::ParseMetaValue(std::string& value) {
  uint32_t tmp; 
  slash::GetFixed32(&value, &tmp);
  ctime_.tv_sec = static_cast<time_t>(tmp);
  slash::GetFixed32(&value, &tmp);
  ctime_.tv_usec = static_cast<suseconds_t>(tmp);
  slash::GetFixed32(&value, &object_count_);

  std::string name;
  for (int i = 0; i < object_count_; i++) {
    slash::GetLengthPrefixedString(&value, &name);
    objects_name_.insert(name);
  }
  assert(objects_name_.size() == object_count_);
  return Status::OK();
}

}  // namespace libzgw
