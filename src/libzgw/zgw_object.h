#ifndef ZGW_OBJECT_H
#define ZGW_OBJECT_H
#include <string>

#include "time.h"
#include "include/slash_status.h"
#include "zgw_user.h"

namespace libzgw {

using slash::Status;

enum ObjectStorageClass {
  kStandard = 0,
};

struct ZgwObjectInfo {
  time_t mtime;
  std::string etag;
  uint64_t size;
  ObjectStorageClass storage_class;
  ZgwUser user;
  ZgwObjectInfo(time_t t, const std::string& et,
      uint64_t s, ObjectStorageClass c, ZgwUser& u)
    : mtime(t), etag(et), size(s),
    storage_class(c), user(u) {
    }
  ZgwObjectInfo()
    : mtime(0), size(0),
    storage_class(ObjectStorageClass::kStandard) {
    }
};

class ZgwObject {
public:
  ZgwObject(const std::string& name);
  ZgwObject(const std::string& name,  const std::string& content,
      const ZgwObjectInfo& i, uint32_t strip_len = 1073741824/* 1 MB */);
  ~ZgwObject();

  std::string name() const {
    return name_;
  }
  void SetName(const std::string& name) {
    name_ = name;
  }
  ZgwObjectInfo info() const {
    return info_;
  }
  std::string content() const {
    return content_;
  }
  uint32_t strip_count() const {
    return strip_count_;
  }

  // Serialization
  std::string MetaKey() const;
  std::string MetaValue() const;
  std::string DataKey(int index) const;
  std::string NextDataStrip(uint32_t* iter) const;
  
  // Deserialization
  Status ParseMetaValue(std::string* value);
  uint32_t ParseStripCount(std::string* value);
  void ParseNextStrip(std::string* value);

private:
  std::string name_;
  std::string content_;
  ZgwObjectInfo info_;
  uint32_t strip_len_;
  uint32_t strip_count_;
};

}

#endif
