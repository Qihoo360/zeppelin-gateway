#include <unistd.h>
#include <set>

#include "zgw_store.h"

#include "include/slash_string.h"

namespace libzgw {

Status ZgwStore::AddObject(const std::string &bucket_name,
                           const std::string &object_name,
                           const ZgwObjectInfo& info,
                           const std::string &content) {
  libzgw::ZgwObject object(object_name, content, info);

  // Set Object Data
  Status s;
  std::string dvalue;
  uint32_t index = 0, iter = 0;
  while (!(dvalue = object.NextDataStrip(&iter)).empty()) {
    s = zp_->Set(kZgwDataTableName, bucket_name + object.DataKey(index++), dvalue);
    if (!s.ok()) {
      return s;
    }
  }

  // Delete Old Data, since the object name may already exist
  std::string ometa;
  libzgw::ZgwObject old_object(object_name);
  s = zp_->Get(kZgwMetaTableName, bucket_name + object.MetaKey(), &ometa);
  if (s.ok()) {
    old_object.ParseMetaValue(&ometa);
    for (uint32_t ti = object.strip_count(); ti < old_object.strip_count(); ti++) {
      zp_->Delete(kZgwDataTableName, bucket_name + old_object.DataKey(ti));
    }
  }

  // Set Object Meta
  s = zp_->Set(kZgwMetaTableName, bucket_name + object.MetaKey(), object.MetaValue());
  if (!s.ok()) {
    return s;
  }

  return s;
}

Status ZgwStore::DelObject(const std::string &bucket_name,
                           const std::string &object_name) {
  // Check meta exist
  ZgwObject object(object_name);
  std::string ob_meta_value;
  Status s = zp_->Get(kZgwMetaTableName, bucket_name + object.MetaKey(), &ob_meta_value);
  if (!s.ok()) {
    return s;
  }

  // Delete subobject if it was a multipart object
  if (!object.part_nums().empty()) {
    for (uint32_t n : object.part_nums()) {
      std::string subobject_name = "__#" + std::to_string(n) + object_name;
      s = DelObject(bucket_name, subobject_name);
      if (!s.ok()) {
        return s;
      }
    }
  }

  // Delete Object Meta
  s = zp_->Delete(kZgwMetaTableName, bucket_name + object.MetaKey());
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
    zp_->Delete(kZgwDataTableName, bucket_name + object.DataKey(index));
  }
  return Status::OK();
}

Status ZgwStore::GetObject(const std::string &bucket_name,
                           const std::string& object_name, ZgwObject* object) {
  object->SetName(object_name);

  // Get Object
  std::string meta_value;
  Status s = zp_->Get(kZgwMetaTableName, bucket_name + object->MetaKey(), &meta_value);
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
    s = zp_->Get(kZgwDataTableName, bucket_name + object->DataKey(index), &cvalue);
    if (!s.ok()) {
      return s;
    }
    object->ParseNextStrip(&cvalue);
  }
  return Status::OK();
}

// Similar to AddObject
Status ZgwStore::InitMultiUpload(std::string &bucket_name, std::string &object_name,
                                 std::string *upload_id, std::string *internal_obname,
                                 ZgwUser *user) {
  Status s;
  timeval now;
  gettimeofday(&now, NULL);
  ZgwObjectInfo ob_info(now, "", 0, kStandard, user->user_info());
  std::string tmp_obname = object_name + std::to_string(time(NULL));
  upload_id->assign(slash::md5(tmp_obname));;
  internal_obname->assign("__" + object_name + *upload_id);
  ZgwObject object(*internal_obname);
  object.SetObjectInfo(ob_info);
  object.SetMultiPartsDone(false);
  object.SetUploadId(*upload_id);

  s = zp_->Set(kZgwMetaTableName, bucket_name + object.MetaKey(), object.MetaValue());
  if (!s.ok()) {
    return s;
  }

  return s;
}

}  // namespace libzgw
