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

  ZgwObjectInfo(timeval t, const std::string& et, uint64_t s,
                ObjectStorageClass c, ZgwUserInfo i)
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
  ZgwObject(const std::string& bucket_name, const std::string& name);
  ZgwObject(const std::string& bucket_name, const std::string& name,
            const std::string& content, const ZgwObjectInfo& i);
  ~ZgwObject() {}

  std::string bucket_name() const {
    return bucket_name_;
  }

  std::string name() const {
    return name_;
  }

  void SetName(const std::string& name) {
    name_ = name;
  }

  void SetBucketName(const std::string& name) {
    bucket_name_ = name;
  }

  void SetObjectInfo(const ZgwObjectInfo &info) {
    info_ = info;
  }

  ZgwObjectInfo &info() {
    return info_;
  }

  const ZgwObjectInfo &info() const {
    return info_;
  }

  const std::string& content() const {
    return content_;
  }

  void AppendContent(const std::string& content) {
    content_.append(content);
  }

  uint32_t strip_count() const {
    return strip_count_;
  }

  std::string upload_id() const {
    return upload_id_;
  }

  void SetUploadId(std::string &v) {
    upload_id_ = v;
  }

  std::set<uint32_t> &part_nums() {
    return part_nums_;
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
  std::string bucket_name_;
  std::string name_;
  std::string content_;
  ZgwObjectInfo info_;
  uint32_t strip_len_;
  uint32_t strip_count_;

  // Multipart Upload
  std::set<uint32_t> part_nums_;
  std::string upload_id_; // md5(object_name + timestamp)

  // Reserve for compatibility
  uint32_t placeholder1_;
  uint32_t placeholder2_;
  uint32_t placeholder3_;
};

}  // namespace libzgw

#endif
