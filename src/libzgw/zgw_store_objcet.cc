#include <assert.h>
#include <unistd.h>
#include <set>

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
  uint32_t index = 0, iter = 0;
  while (!(dvalue = object.NextDataStrip(&iter)).empty()) {
    usleep(500);
    s = zp_->Set(bucket_name, object.DataKey(index++), dvalue);
    if (!s.ok()) {
      return s;
    }
  }

  // Add to Objects List
  // Get object name list
  std::string meta_value;
  s = zp_->Get(bucket_name, ZgwBucket::MetaKey(bucket_name), &meta_value);
  if (!s.ok()) {
    return s;
  }
  // Not used here
  uint32_t tmp1, tmp2;
  slash::GetFixed32(&meta_value, &tmp1);
  slash::GetFixed32(&meta_value, &tmp2);

  // Parse objects count and name;
  uint32_t name_count;
  std::set<std::string> name_list;
  slash::GetFixed32(&meta_value, &name_count);
  for (size_t i = 0; i < name_count; i++) {
    std::string name;
    slash::GetLengthPrefixedString(&meta_value, &name);
    name_list.insert(name);
  }
  assert(name_count == name_list.size());

  // Insert new object name
  name_list.insert(object.name());

  // Put back name list to bucket meta
  meta_value.clear();
  slash::PutFixed32(&meta_value, tmp1);
  slash::PutFixed32(&meta_value, tmp2);

  slash::PutFixed32(&meta_value, name_list.size());
  for (auto &name : name_list) {
    slash::PutLengthPrefixedString(&meta_value, name);
  }
  s = zp_->Set(bucket_name, ZgwBucket::MetaKey(bucket_name), meta_value);

  // Set Object Meta
  s = zp_->Set(bucket_name, object.MetaKey(), object.MetaValue());
  return s;
}

Status ZgwStore::DelObject(const std::string &bucket_name,
    const std::string &object_name) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Check meta exist
  ZgwObject object(object_name);
  std::string ob_meta_value;
  s = zp_->Get(bucket_name, object.MetaKey(), &ob_meta_value);
  if (!s.ok()) {
    return s;
  }

  // Delete from Objects List
  // Get object name list
  std::string meta_value;
  s = zp_->Get(bucket_name, ZgwBucket::MetaKey(bucket_name), &meta_value);
  // Not used here
  uint32_t tmp1, tmp2;
  slash::GetFixed32(&meta_value, &tmp1);
  slash::GetFixed32(&meta_value, &tmp2);

  // Parse objects count and name;
  uint32_t name_count;
  std::set<std::string> name_list;
  slash::GetFixed32(&meta_value, &name_count);
  for (size_t i = 0; i < name_count; i++) {
    std::string name;
    slash::GetLengthPrefixedString(&meta_value, &name);
    name_list.insert(name);
  }
  assert(name_count == name_list.size());

  // Delete object name
  name_list.erase(object.name());

  // Put back name list to bucket meta
  meta_value.clear();
  slash::PutFixed32(&meta_value, tmp1);
  slash::PutFixed32(&meta_value, tmp2);

  slash::PutFixed32(&meta_value, name_list.size());
  for (auto &name : name_list) {
    slash::PutLengthPrefixedString(&meta_value, name);
  }
  s = zp_->Set(bucket_name, ZgwBucket::MetaKey(bucket_name), meta_value);

  // Delete Object Meta
  s = zp_->Delete(bucket_name, object.MetaKey());
  if (!s.ok()) {
    return s;
  }

  // Parse from value
  s = object.ParseMetaValue(&ob_meta_value);
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

}  // namespace libzgw
