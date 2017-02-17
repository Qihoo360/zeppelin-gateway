#include "zgw_object.h"

namespace libzgw {

static const std::string kObjectMetaPrefix = "__O";
static const std::string kObjectDataPrefix = "__o";

ZgwObject::ZgwObject(const ZgwObjectInfo& i,
    const std::string& c, uint32_t s)
  :info_(i),
  content_(c),
  strip_len_(s) {
  }

ZgwObject::~ZgwObject() {
}

std::string ZgwObject::MetaKey() const {
  return kObjectMetaPrefix + info_.key;
}

std::string ZgwObject::MetaValue() const {
  return std::string();
}

std::string ZgwObject::DataKey(int index) const {
  return kObjectDataPrefix + info_.key + std::to_string(index);
}

std::string ZgwObject::NextDataStrip(uint32_t* iter) const {
  if (*iter > content_.size()) {
    return std::string();
  }
  uint32_t clen = content_.size() - *iter;
  clen = (strip_len_ < clen) ? strip_len_ : clen;
  return content_.substr((*iter)++, clen);
}



}
