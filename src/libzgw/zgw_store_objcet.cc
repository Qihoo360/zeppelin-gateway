#include "zgw_store.h"

#include <unistd.h>
#include <set>

#include "include/slash_string.h"
#include <openssl/md5.h>

namespace libzgw {

Status ZgwStore::AddObject(ZgwObject& object) {
  // Set Object Data
  Status s;
  std::string dvalue;
  uint32_t index = 0, iter = 0;
  while (!(dvalue = object.NextDataStrip(&iter)).empty()) {
    s = zp_->Set(kZgwDataTableName, object.DataKey(index++), dvalue);
    if (!s.ok()) {
      return s;
    }
  }

  // Delete Old Data, since the object name may already exist
  std::string ometa;
  libzgw::ZgwObject old_object(object.bucket_name(), object.name());
  s = zp_->Get(kZgwMetaTableName, object.MetaKey(), &ometa);
  if (s.ok()) {
    old_object.ParseMetaValue(&ometa);
    for (uint32_t ti = object.strip_count(); ti < old_object.strip_count(); ti++) {
      zp_->Delete(kZgwDataTableName, old_object.DataKey(ti));
    }
  }

  // Set Object Meta
  return zp_->Set(kZgwMetaTableName, object.MetaKey(), object.MetaValue());
}

Status ZgwStore::DelObject(const std::string &bucket_name,
                           const std::string &object_name) {
  // Check meta exist
  ZgwObject object(bucket_name, object_name);
  std::string ob_meta_value;
  Status s = zp_->Get(kZgwMetaTableName, object.MetaKey(), &ob_meta_value);
  if (!s.ok()) {
    return s;
  }

  // Delete subobject if it was a multipart object
  std::string internal_obname = "__" + object.name() + object.upload_id();
  for (uint32_t n : object.part_nums()) {
    std::string subobject_name = "__#" + std::to_string(n) + internal_obname;
    s = DelObject(bucket_name, subobject_name);
    if (!s.ok()) {
      return s;
    }
  }

  // Delete Object Meta
  s = zp_->Delete(kZgwMetaTableName, object.MetaKey());
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
    zp_->Delete(kZgwDataTableName, object.DataKey(index));
  }
  return Status::OK();
}

Status ZgwStore::GetObject(ZgwObject* object, bool need_content) {
  // Get Object
  std::string meta_value;
  Status s = zp_->Get(kZgwMetaTableName, object->MetaKey(), &meta_value);
  if (!s.ok()) {
    return s;
  }

  // Parse from value
  s = object->ParseMetaValue(&meta_value);
  if (!s.ok()) {
    return s;
  }

  if (need_content) {
    for (auto n : object->part_nums()) {
      std::string internal_obname = "__" + object->name() + object->upload_id();
      std::string subobject_name = "__#" + std::to_string(n) + internal_obname;
      ZgwObject subobject(object->bucket_name(), subobject_name);
      s = GetObject(&subobject, true);
      if (!s.ok()) {
        return s;
      }
      object->AppendContent(subobject.content());
    }
    // Get Object Data
    uint32_t index = 0;
    std::string cvalue;
    for (; index < object->strip_count(); index++) {
      s = zp_->Get(kZgwDataTableName, object->DataKey(index), &cvalue);
      if (!s.ok()) {
        return s;
      }
      object->ParseNextStrip(&cvalue);
    }
  }

  return Status::OK();
}

Status ZgwStore::UploadPart(const std::string& bucket_name, const std::string& internal_obname,
                            const ZgwObjectInfo& info, const std::string& content, int part_num) {
  // Get multipart object meta
  ZgwObject object(bucket_name, internal_obname);
  Status s = GetObject(&object, false);
  if (!s.ok()) {
    return s;
  }
  std::string subobject_name = "__#" + std::to_string(part_num) + internal_obname;

  // Check if need override
  auto &part_nums = object.part_nums();
  if (part_nums.find(part_num) != part_nums.end()) {
    s = DelObject(bucket_name, subobject_name);
    if (!s.ok()) {
      return s;
    }
  }

  ZgwObject part_object(bucket_name, subobject_name, content, info);
  s = AddObject(part_object);
  if (!s.ok()) {
    return s;
  }

  // Update multipart object meta
  part_nums.insert(part_num);
  return zp_->Set(kZgwMetaTableName, object.MetaKey(), object.MetaValue());
}

Status ZgwStore::ListParts(const std::string& bucket_name, const std::string& internal_obname,
                           std::vector<std::pair<int, ZgwObject>> *parts) {
  // Get multipart object meta
  libzgw::ZgwObject object(bucket_name, internal_obname);
  Status s = GetObject(&object, false);
  if (!s.ok()) {
    return s;
  }

  for (auto n : object.part_nums()) {
    std::string subobject_name = "__#" + std::to_string(n) + internal_obname;
    ZgwObject subobject(bucket_name, subobject_name);
    s = GetObject(&subobject, false);
    if (!s.ok()) {
      return s;
    }
    parts->push_back(std::make_pair(n, subobject));
  }

  return Status::OK();
}

Status ZgwStore::CompleteMultiUpload(const std::string& bucket_name,
                                     const std::string& internal_obname) {
  std::string final_object_name = internal_obname.substr(2, internal_obname.size() - 32 - 2);
  std::string final_etag;
  int final_size = 0;
  MD5_CTX md5_ctx;
  char buf[33] = {0};
  unsigned char md5[16] = {0};
  MD5_Init(&md5_ctx);

  // Calculate final size and etag
  Status s;
  std::vector<std::pair<int, ZgwObject>> parts;
  ListParts(bucket_name, internal_obname, &parts);
  for (auto &it : parts) {
    ZgwObject subob(it.second); // Intend Use Copy
    s = GetObject(&subob, true);
    if (!s.ok()) {
      return s;
    }
    final_size += subob.info().size;
    MD5_Update(&md5_ctx, subob.content().c_str(), subob.content().size());
  }
  MD5_Final(md5, &md5_ctx);
  for (int i = 0; i < 16; i++) {
    sprintf(buf + i * 2, "%02x", md5[i]);
  }
  final_etag.assign("\"" + std::string(buf) + "\"");

  ZgwObject cur_object(bucket_name, internal_obname);
  // Get stored initial object info
  s = GetObject(&cur_object, false); 
  if (!s.ok()) {
    return s;
  }
  // Upload complete object info
  ZgwObject final_object(cur_object);
  timeval now;
  gettimeofday(&now, NULL);
  final_object.SetName(final_object_name);
  final_object.info().mtime = now;
  final_object.info().size = final_size;
  final_object.info().etag = final_etag;

  // Set new meta
  s = zp_->Set(kZgwMetaTableName, final_object.MetaKey(), final_object.MetaValue());
  if (!s.ok()) {
    return s;
  }
  // Delete old meta
  s = zp_->Delete(kZgwMetaTableName, cur_object.MetaKey());
  if (!s.ok() && !s.IsNotFound()) {
    return s;
  }

  return Status::OK();
}

}  // namespace libzgw
