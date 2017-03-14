#ifndef ZGW_OBJECT_H
#define ZGW_OBJECT_H
#include <string>
#include <sys/time.h>

#include "include/slash_status.h"
#include "zgw_user.h"
#include "zgw_namelist.h"

namespace libzgw {

using slash::Status;

enum ObjectStorageClass {
  kStandard = 0,
};

struct ZgwObjectInfo {
  timeval mtime;
  std::string etag;
  uint64_t size;
  ObjectStorageClass storage_class;
  ZgwUserInfo user;

  ZgwObjectInfo(timeval t, const std::string& et,
      uint64_t s, ObjectStorageClass c, ZgwUserInfo i)
    : mtime(t), etag(et), size(s),
    storage_class(c), user(i) {
  }
  ZgwObjectInfo()
    : size(0),
    storage_class(ObjectStorageClass::kStandard) {
  }
  std::string MetaValue() const;
  Status ParseMetaValue(std::string *meta_value);
};

class ZgwObject {
 public:
  ZgwObject(const std::string& name);
  ZgwObject(const std::string& name, const std::string& content,
            const ZgwObjectInfo& i, uint32_t strip_len = 8388608 /* 8 MB */);
  ~ZgwObject();

  std::string name() const {
    return name_;
  }

  void SetName(const std::string& name) {
    name_ = name;
  }

  void SetObjectInfo(const ZgwObjectInfo &info) {
    info_ = info;
  }

  const ZgwObjectInfo &info() const {
    return info_;
  }

  std::string content() const {
    return content_;
  }

  uint32_t strip_count() const {
    return strip_count_;
  }

  bool multiparts_done() const {
    return multiparts_done_;
  }

  void SetMultiPartsDone(bool v) {
    multiparts_done_ = v;
  }

  bool is_partial() const {
    return is_partial_;
  }

  void SetIsPartial(bool v) {
    is_partial_ = v;
  }

  std::string upload_id() const {
    return upload_id_;
  }

  void SetUploadId(std::string &v) {
    upload_id_ = v;
  }

  // Serialization
  std::string MetaKey() const;
  std::string MetaValue() const;
  std::string DataKey(int index) const;
  std::string NextDataStrip(uint32_t* iter) const;
  
  // Deserialization
  Status ParseMetaValue(std::string* value);
  void ParseNextStrip(std::string* value);

 private:
  std::string name_;
  std::string content_;
  ZgwObjectInfo info_;
  uint32_t strip_len_;
  uint32_t strip_count_;

  // Multipart Upload
  // for virtual object
  bool multiparts_done_;
  std::set<int> part_nums_;
  std::string upload_id_; // md5(object_name) + timestamp
  // for object part
  bool is_partial_;
  uint32_t part_num_;
};

}  // namespace libzgw

#endif
