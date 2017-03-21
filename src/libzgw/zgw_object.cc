#include "zgw_object.h"
#include "include/slash_coding.h"

namespace libzgw {

static const std::string kObjectMetaPrefix = "__O__";
static const std::string kObjectDataPrefix = "__o";
static const std::string kObjectDataSep = "__";
static const int kObjectDataStripLen = 1048576; // 1 MB

ZgwObject::ZgwObject(const std::string& name)
      : name_(name),
        strip_len_(0),
        strip_count_(0),
        multiparts_done_(true),
        is_partial_(false) {
}

ZgwObject::ZgwObject(const std::string& name, const std::string& content,
                     const ZgwObjectInfo& i)
      : name_(name),
        content_(content),
        info_(i),
        strip_len_(kObjectDataStripLen),
        multiparts_done_(true),
        is_partial_(false) {
  int m = content_.size() % strip_len_;
  strip_count_ = content_.empty() ? 0 :
    (content_.size() / strip_len_ + (m > 0 ? 1 : 0));
}

ZgwObject::~ZgwObject() {
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
  std::string multipart_sign = is_partial_ ? std::to_string(part_num_) : "";
  return kObjectMetaPrefix + name_ + multipart_sign;
}

std::string ZgwObject::MetaValue() const {
  std::string result;
  // Object Internal meta
  slash::PutFixed32(&result, strip_count_);
  uint32_t b = static_cast<uint32_t>(multiparts_done_);
  slash::PutFixed32(&result, b);
  slash::PutFixed32(&result, part_nums_.size());
  for (auto &i : part_nums_) {
    slash::PutFixed32(&result, i);
  }
  // Partial info
  b = static_cast<uint32_t>(is_partial_);
  slash::PutFixed32(&result, b);
  slash::PutFixed32(&result, part_num_);
  slash::PutLengthPrefixedString(&result, upload_id_);

  // Object Info
  slash::PutLengthPrefixedString(&result, info_.MetaValue());
  return result;
}

std::string ZgwObject::DataKey(int index) const {
  return kObjectDataPrefix + std::to_string(index) + kObjectDataSep + name_;
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
  uint32_t b;
  slash::GetFixed32(value, &b);
  multiparts_done_ = static_cast<bool>(b);
  uint32_t n, v;
  slash::GetFixed32(value, &n);
  for (uint32_t i; i < n; i++) {
    slash::GetFixed32(value, &v);
    part_nums_.insert(v);
  }
  // Partial info
  slash::GetFixed32(value, &b);
  is_partial_ = static_cast<bool>(b);
  slash::GetFixed32(value, &part_num_);
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
