#ifndef ZGW_OBJECT_H
#define ZGW_OBJECT_H
#include <string>

#include "time.h"

#include "zgw_user.h"

namespace libzgw {

enum ObjectStorageClass {
  kStandard = 0,
};

struct ZgwObjectInfo {
  std::string key;
  time_t mtime;
  std::string etag;
  uint64_t size;
  ObjectStorageClass storage_class;
  ZgwUser user;
  ZgwObjectInfo(const std::string& k, time_t t, const std::string& et,
      uint64_t s, ObjectStorageClass c, ZgwUser& u)
    : key(k), mtime(t), etag(et), size(s),
    storage_class(c), user(u) {
    }
};

class ZgwObject {
public:
  ZgwObject(const ZgwObjectInfo& i, const std::string& c,
      uint32_t strip_len);
  ~ZgwObject();

  ZgwObjectInfo info() const {
    return info_;
  }
  std::string content() const {
    return content_;
  }

  std::string MetaKey() const;
  std::string MetaValue() const;
  std::string DataKey(int index) const;
  std::string NextDataStrip(uint32_t* iter) const;

private:
  ZgwObjectInfo info_;
  std::string content_;
  uint32_t strip_len_;
};

}

#endif
