#include "zgw_object.h"
#include "include/slash_coding.h"

namespace libzgw {

static const std::string kObjectMetaPrefix = "__O__";
static const std::string kObjectDataPrefix = "__o";
static const std::string kObjectDataSep = "__";

ZgwObject::ZgwObject(const std::string& name)
  :name_(name) {
  }

ZgwObject::ZgwObject(const std::string& name, const std::string& content,
      const ZgwObjectInfo& i, uint32_t strip_len)
  :name_(name),
  content_(content),
  info_(i),
  strip_len_(strip_len) {
    strip_count_ = content_.size() / strip_len_ + 1;
  }

ZgwObject::~ZgwObject() {
}

std::string ZgwObject::MetaKey() const {
  return kObjectMetaPrefix + name_;
}

std::string ZgwObject::MetaValue() const {
  std::string result;
  // Object Internal meta
  slash::PutFixed32(&result, strip_count_);

  // Object Info
  slash::PutFixed64(&result, info_.mtime);
  slash::PutLengthPrefixedString(&result,  info_.etag);
  slash::PutFixed64(&result, info_.size);
  slash::PutFixed64(&result, info_.storage_class);
  slash::PutLengthPrefixedString(&result,  info_.user.MetaValue());
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
  
  // Object info
  uint64_t tmp; 
  slash::GetFixed64(value, &tmp);
  info_.mtime = static_cast<time_t>(tmp); // mtime
  bool res = slash::GetLengthPrefixedString(value, &(info_.etag)); // etag
  if (!res) {
    return Status::Corruption("Parse info etag failed");
  }
  slash::GetFixed64(value, &(info_.size)); // size
  slash::GetFixed64(value, &tmp);
  info_.storage_class = static_cast<ObjectStorageClass>(tmp); // storage_class
  std::string user_value;
  res = slash::GetLengthPrefixedString(value, &user_value); //user meta value
  if (!res) {
    return Status::Corruption("Parse user meta value failed");
  }
  return info_.user.ParseMetaValue(&user_value);
}

void ZgwObject::ParseNextStrip(std::string* value) {
  content_.append(*value);
}

}
