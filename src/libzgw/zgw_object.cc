#include "zgw_object.h"
#include "include/slash_coding.h"

namespace libzgw {

static const std::string kObjectMetaPrefix = "__O__";
static const std::string kObjectDataPrefix = "__o";
static const std::string kObjectDataSep = "__";
static const int kObjectDataStripLen = 1048576; // 1 MB
extern std::string kBucketMetaPrefix;

ZgwObject::ZgwObject(const std::string& bucket_name, const std::string& name)
      : bucket_name_(bucket_name),
        name_(name),
        strip_len_(0),
        strip_count_(0) {
}

ZgwObject::ZgwObject(const std::string& bucket_name, const std::string& name,
                     const std::string& content, const ZgwObjectInfo& i)
      : bucket_name_(bucket_name),
        name_(name),
        content_(content),
        info_(i),
        strip_len_(kObjectDataStripLen) {
  strip_count_ = content_.empty() ? 0 : content_.size() / strip_len_ + 1;
}

std::string ZgwObjectInfo::MetaValue() const {
  std::string result;
  slash::PutFixed64(&result, mtime.tv_sec);
  slash::PutFixed64(&result, mtime.tv_usec);
  slash::PutLengthPrefixedString(&result, etag);
  slash::PutFixed64(&result, size);
  slash::PutFixed64(&result, storage_class);
  slash::PutLengthPrefixedString(&result, user.MetaValue());
  return result;
}

Status ZgwObjectInfo::ParseMetaValue(std::string *value) {
  uint64_t tmp; 
  slash::GetFixed64(value, &tmp);
  mtime.tv_sec = static_cast<time_t>(tmp); // mtime.tv_sec
  slash::GetFixed64(value, &tmp);
  mtime.tv_usec = static_cast<time_t>(tmp); // mtime.tv_usec
  bool res = slash::GetLengthPrefixedString(value, &etag); // etag
  if (!res) {
    return Status::Corruption("Parse info etag failed");
  }
  slash::GetFixed64(value, &size); // size
  slash::GetFixed64(value, &tmp);
  storage_class = static_cast<ObjectStorageClass>(tmp); // storage_class
  std::string user_value;
  res = slash::GetLengthPrefixedString(value, &user_value); //user meta value
  if (!res) {
    return Status::Corruption("Parse user meta value failed");
  }
  return user.ParseMetaValue(&user_value);
}

std::string ZgwObject::MetaKey() const {
  return kBucketMetaPrefix + bucket_name_ + kObjectMetaPrefix + name_;
}

std::string ZgwObject::MetaValue() const {
  std::string result;
  // Object Internal meta
  slash::PutFixed32(&result, strip_count_);
  slash::PutFixed32(&result, part_nums_.size());
  for (uint32_t i : part_nums_) {
    slash::PutFixed32(&result, i);
  }
  slash::PutLengthPrefixedString(&result, upload_id_);

  // Object Info
  slash::PutLengthPrefixedString(&result, info_.MetaValue());
  return result;
}

std::string ZgwObject::DataKey(int index) const {
  return kBucketMetaPrefix + bucket_name_ +
    kObjectDataPrefix + std::to_string(index) +
    kObjectDataSep + name_;
}

std::string ZgwObject::NextDataStrip(uint32_t* iter) const {
  if (*iter > content_.size()) {
    return std::string();
  }
  uint32_t clen = content_.size() - *iter;
  clen = (clen > strip_len_) ? strip_len_ : clen;
  *iter += clen;
  return content_.substr(*iter - clen, clen);
}

Status ZgwObject::ParseMetaValue(std::string* value) {
  // Object Interal meta
  slash::GetFixed32(value, &strip_count_);
  uint32_t n, v;
  slash::GetFixed32(value, &n);
  for (uint32_t i = 0; i < n; i++) {
    slash::GetFixed32(value, &v);
    part_nums_.insert(v);
  }
  bool res = slash::GetLengthPrefixedString(value, &upload_id_);
  if (!res) {
    return Status::Corruption("Parse upload_id failed");
  }

  // Object info
  std::string ob_meta;
  res = slash::GetLengthPrefixedString(value, &ob_meta);
  if (!res) {
    return Status::Corruption("Parse ob_meta failed");
  }
  return info_.ParseMetaValue(&ob_meta);
}

void ZgwObject::ParseNextStrip(std::string* value) {
  content_.append(*value);
}

}
