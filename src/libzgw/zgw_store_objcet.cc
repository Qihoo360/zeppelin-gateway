#include "src/libzgw/zgw_store.h"

#include <unistd.h>
#include <set>

#include <openssl/md5.h>
#include "slash/include/slash_string.h"

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
  for (uint32_t n : object.part_nums()) {
    std::string internal_obname = object.name();
    if (object_name.find_first_of(kInternalObjectNamePrefix) != 0) {
      internal_obname = kInternalObjectNamePrefix + object.name() + object.upload_id();
    }
    std::string subobject_name = kInternalSubObjectNamePrefix +
      std::to_string(n) + internal_obname;
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

Status ZgwStore::GetPartialObject(ZgwObject* object, int start_byte, int partial_size) {
  // Assert object has parsed

  std::vector<ZgwObject> candidate_object;
  Status s;
  if (object->part_nums().empty()) {
    // This object
    int start_strip = start_byte / object->strip_len();
    start_byte -= start_strip * object->strip_len();
    int m = (start_byte + partial_size) % object->strip_len();
    int strip_needed = (start_byte + partial_size) / object->strip_len() + (m > 0 ? 1 : 0);
    std::string candidate_value, svalue;
    for (int i = start_strip; i < start_strip + strip_needed; ++i) {
      s = zp_->Get(kZgwDataTableName, object->DataKey(i), &svalue);
      if (!s.ok()) {
        return s;
      }
      candidate_value.append(svalue);
    }
    object->AppendContent(candidate_value.substr(start_byte, partial_size));
  } else {
    for (auto n : object->part_nums()) {
      // Get sub object size;
      std::string subobject_name = kInternalSubObjectNamePrefix +
        std::to_string(n) + kInternalObjectNamePrefix + object->name() + object->upload_id();
      ZgwObject subobject(object->bucket_name(), subobject_name);
      s = GetObject(&subobject, false);
      if (!s.ok()) {
        return s;
      }
      int cur_object_size = subobject.info().size;

      if (start_byte < cur_object_size) {
        // This subobject
        int get_size = std::min(cur_object_size - start_byte, partial_size);
        s = GetPartialObject(&subobject, start_byte, get_size);
        if (!s.ok()) {
          return s;
        }
        object->AppendContent(subobject.content());
        partial_size -= get_size;
        start_byte = 0;
        if (partial_size == 0) {
          break;
        }
      } else {
        // Next subobject
        start_byte -= cur_object_size;
      }
    }
  }

  return Status::OK();
}

Status ZgwStore::GetPartialObject(ZgwObject* object,
                                  std::vector<std::pair<int, uint32_t>>& segments) {
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

  for (auto &seg : segments) {
    // Calculate partial size and start byte
    uint32_t partial_size = 0, start_byte = 0;
    if (seg.first < 0) {
      partial_size = 0 - seg.first;
      partial_size = std::min((uint64_t)partial_size, object->info().size);
      start_byte = object->info().size - partial_size;
      seg.first = start_byte;
      seg.second = start_byte + partial_size - 1;
    } else {
      seg.second = std::min((uint64_t)seg.second, object->info().size - 1);
      partial_size = seg.second - seg.first + 1;
      start_byte = seg.first;
    }

    if (start_byte > object->info().size) {
      return Status::EndFile("Start byte is greater than object size");
    }

    Status s = GetPartialObject(object, start_byte, partial_size);
    if (!s.ok()) {
      return s;
    }
  }

  return Status::OK();
}

Status ZgwStore::ListObjects(const std::string& bucket_name,
                             const std::vector<std::string>& candidate_names,
                             std::vector<ZgwObject>* objects) {
  assert(objects);
  Status s;
  std::vector<std::string> keys;
  std::map<std::string, std::string> values;
  for (auto& name : candidate_names) {
    ZgwObject o(bucket_name, name);
    keys.push_back(o.MetaKey());
    objects->push_back(std::move(o));
  }
  s = zp_->Mget(kZgwMetaTableName, keys, &values);
  if (!s.ok()) {
    return s;
  }

  for (auto& o : *objects) {
    std::string meta_value = values[o.MetaKey()];
    s = o.ParseMetaValue(&meta_value);
    if (!s.ok()) {
      return s;
    }
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
      std::string subobject_name = object->name();
      if (subobject_name.find_first_of(kInternalObjectNamePrefix) != 0) {
        std::string internal_obname = kInternalObjectNamePrefix +
          subobject_name + object->upload_id();
        subobject_name = kInternalSubObjectNamePrefix + std::to_string(n) + internal_obname;
      }
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
  std::string subobject_name = kInternalSubObjectNamePrefix +
    std::to_string(part_num) + internal_obname;

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

  std::vector<std::string> keys;
  std::map<std::string, std::string> values;
  for (auto n : object.part_nums()) {
    std::string subobject_name = kInternalSubObjectNamePrefix +
      std::to_string(n) + internal_obname;
    ZgwObject subobject(bucket_name, subobject_name);
    keys.push_back(subobject.MetaKey());
    parts->push_back(std::make_pair(n, std::move(subobject)));
  }

  s = zp_->Mget(kZgwMetaTableName, keys, &values);
  if (!s.ok()) {
    return s;
  }

  for (auto& p : *parts) {
    std::string meta_value = values[p.second.MetaKey()];
    s = p.second.ParseMetaValue(&meta_value);
    if (!s.ok()) {
      return s;
    }
  }

  return Status::OK();
}

Status ZgwStore::CompleteMultiUpload(const std::string& bucket_name,
                                     const std::string& internal_obname,
                                     const std::vector<std::pair<int, ZgwObject>>& parts,
                                     std::string *final_etag) {
  std::string final_object_name = internal_obname.substr(2, internal_obname.size() - 32 - 2);
  int final_size = 0;
  MD5_CTX md5_ctx;
  char buf[33] = {0};
  unsigned char md5[16] = {0};
  MD5_Init(&md5_ctx);

  // Calculate final size and etag
  Status s;
  for (auto &it : parts) {
    ZgwObject subob(it.second); // Intend Use Copy
    s = GetObject(&subob, false);
    if (!s.ok()) {
      return s;
    }
    final_size += subob.info().size;
    MD5_Update(&md5_ctx, subob.info().etag.c_str(), subob.info().etag.size());
  }
  MD5_Final(md5, &md5_ctx);
  for (int i = 0; i < 16; i++) {
    sprintf(buf + i * 2, "%02x", md5[i]);
  }
  final_etag->assign("\"" + std::string(buf) + "\"");

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
  final_object.info().etag = *final_etag;

  // Set new meta
  s = AddObject(final_object);
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
