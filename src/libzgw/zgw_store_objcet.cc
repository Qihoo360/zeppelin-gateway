#include <unistd.h>
#include <set>

#include "zgw_store.h"

#include "include/slash_string.h"

namespace libzgw {

Status ZgwStore::AddObject(const std::string &access_key,
                           const std::string &bucket_name,
                           const std::string &object_name,
                           const ZgwObjectInfo& info,
                           const std::string &content) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Auth
  ZgwUser *user;
  s = GetUser(access_key, &user);
  if (!s.ok()) {
    return s;
  }

  libzgw::ZgwObject object(object_name, content, info);
  object.SetUserInfo(user->user_info());

  // Set Object Data
  std::string dvalue;
  uint32_t index = 0, iter = 0;
  while (!(dvalue = object.NextDataStrip(&iter)).empty()) {
    s = zp_->Set(bucket_name, object.DataKey(index++), dvalue);
    if (!s.ok()) {
      return s;
    }
  }

  // Delete Old Data, since the object name may already exist
  std::string ometa;
  libzgw::ZgwObject old_object(object_name);
  s = zp_->Get(bucket_name, object.MetaKey(), &ometa);
  if (s.ok()) {
    old_object.ParseMetaValue(&ometa);
    for (uint32_t ti = object.strip_count(); ti < old_object.strip_count(); ti++) {
      zp_->Delete(bucket_name, old_object.DataKey(ti));
    }
  }

  // Set Object Meta
  s = zp_->Set(bucket_name, object.MetaKey(), object.MetaValue());
  if (!s.ok()) {
    return s;
  }

  // // Add objects to name list in bucket
  // std::string meta_value;
  // ZgwBucket bucket(bucket_name);
  // s = zp_->Get(bucket_name, bucket.MetaKey(), &meta_value);
  // if (!s.ok()) {
  //   return s;
  // }
  // bucket.ParseMetaValue(meta_value);
  // bucket.AddObject(object.name());
  // // Update bucket meta value
  // s = zp_->Set(bucket_name, bucket.MetaKey(), bucket.MetaValue());

  return s;
}

Status ZgwStore::DelObject(const std::string &access_key,
                           const std::string &bucket_name,
                           const std::string &object_name) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Auth
  ZgwUser *user;
  s = GetUser(access_key, &user);
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

  // // Delete objects from name list in bucket
  // std::string meta_value;
  // ZgwBucket bucket(bucket_name);
  // s = zp_->Get(bucket_name, bucket.MetaKey(), &meta_value);
  // if (!s.ok()) {
  //   return s;
  // }
  // bucket.ParseMetaValue(meta_value);
  // bucket.DelObject(object_name);
  // // Update bucket meta value
  // s = zp_->Set(bucket_name, bucket.MetaKey(), bucket.MetaValue());
  // if (!s.ok()) {
  //   return s;
  // }

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

Status ZgwStore::GetObject(const std::string &access_key,
                           const std::string &bucket_name,
                           const std::string& object_name, ZgwObject* object) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }
  object->SetName(object_name);

  // Auth
  ZgwUser *user;
  s = GetUser(access_key, &user);
  if (!s.ok()) {
    return s;
  }

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
