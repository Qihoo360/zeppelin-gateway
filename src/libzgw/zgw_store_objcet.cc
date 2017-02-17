#include "zgw_store.h"

#include "include/slash_string.h"

namespace libzgw {
 
Status ZgwStore::AddObject(const std::string &bucket_name,
    const ZgwObject& object) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Set Object Data
  std::string dvalue;
  uint32_t iter = 0, index = 0;
  while (!(dvalue = object.NextDataStrip(&iter)).empty()) {
    s = zp_->Set(bucket_name, object.DataKey(index++), dvalue);
    if (!s.ok()) {
      return s;
    }
  }

  // Set Object Meta
  return zp_->Set(bucket_name, object.MetaKey(), object.MetaValue());
}

Status ZgwStore::DelObject(const std::string &bucket_name,
    const std::string &object_name) {
  return Status::OK();
}
Status ZgwStore::ListObjects(const std::string &bucket_name,
    std::vector<ZgwObject>* buckets) {
  return Status::OK();
}

}
