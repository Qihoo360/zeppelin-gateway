#include <sys/time.h>
#include <assert.h>

#include "zgw_bucket.h"
#include "include/slash_coding.h"

namespace libzgw {

static const std::string kBucketMetaPrefix = "__B__";

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
  slash::PutLengthPrefixedString(&result, user_info_.MetaValue());
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
  std::string user_meta;
  bool res = slash::GetLengthPrefixedString(&value, &user_meta);
  if (!res) {
    return Status::Corruption("Parse user_meta failed");
  }
  Status s = user_info_.ParseMetaValue(&user_meta);
  if (!s.ok()) {
    return s;
  }
  slash::GetFixed32(&value, &tmp);
  ctime_.tv_sec = static_cast<time_t>(tmp);
  slash::GetFixed32(&value, &tmp);
  ctime_.tv_usec = static_cast<suseconds_t>(tmp);

  uint32_t object_count_;
  std::string name;
  slash::GetFixed32(&value, &object_count_);
  for (size_t i = 0; i < object_count_; i++) {
    res = slash::GetLengthPrefixedString(&value, &name);
    if (!res) {
      return Status::Corruption("Parse object name failed");
    }
    objects_name_.insert(name);
  }
  assert(objects_name_.size() == object_count_);
  return Status::OK();
}

}  // namespace libzgw
