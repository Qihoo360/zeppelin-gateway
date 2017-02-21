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
  uint32_t iter = 0;
  while (!(dvalue = object.NextDataStrip(&iter)).empty()) {
    s = zp_->Set(bucket_name, object.DataKey(iter - 1), dvalue);
    if (!s.ok()) {
      return s;
    }
  }

  // Set Object Meta
  return zp_->Set(bucket_name, object.MetaKey(), object.MetaValue());
}

Status ZgwStore::DelObject(const std::string &bucket_name,
    const std::string &object_name) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Check meta exist
  ZgwObject object(object_name);
  std::string meta_value;
  s = zp_->Get(bucket_name, object.MetaKey(), &meta_value);
  if (!s.ok()) {
    return s;
  }

  // Delete Object Meta
  s = zp_->Delete(bucket_name, object.MetaKey());
  if (!s.ok()) {
    return s;
  }

  // Parse from value
  s = object.ParseMetaValue(&meta_value);
  if (!s.ok()) {
    return s;
  }

  // Delete Object Data
  uint32_t index = 0;
  for (; index < object.strip_count(); index++) {
    zp_->Delete(bucket_name, object.DataKey(index));
  }
  return Status::OK();
}

Status ZgwStore::GetObject(const std::string &bucket_name,
    const std::string& object_name, ZgwObject* object) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }
  object->SetName(object_name);

  // Get Object
  std::string meta_value;
  s = zp_->Get(bucket_name, object->MetaKey(), &meta_value);
  if (!s.ok()) {
    return s;
  }

  // Parse from value
  s = object->ParseMetaValue(&meta_value);
  if (!s.ok()) {
    return s;
  }

  // Get Object Data
  uint32_t index = 0;
  std::string cvalue;
  for (; index < object->strip_count(); index++) {
    s = zp_->Get(bucket_name, object->DataKey(index), &cvalue);
    if (!s.ok()) {
      return s;
    }
    object->ParseNextStrip(&cvalue);
  }
  return Status::OK();
}

}
