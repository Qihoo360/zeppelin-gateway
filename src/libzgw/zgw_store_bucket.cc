#include <unistd.h>

#include "zgw_store.h"
#include "zgw_user.h"

#include "include/slash_string.h"

namespace libzgw {
 
Status ZgwStore::AddBucket(const std::string &bucket_name, ZgwUserInfo user_info) {
  // Add Bucket Meta
  ZgwBucket bucket(bucket_name);
  int retry = 3;
  bucket.SetUserInfo(user_info);
  return zp_->Set(kZgwMetaTableName, bucket.MetaKey(), bucket.MetaValue());
}

Status ZgwStore::SaveNameList(const std::string &meta_key,
                              const std::string &meta_value) {
  Status s = zp_->Set(kZgwMetaTableName, meta_key, meta_value);
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

Status ZgwStore::GetNameList(const std::string &meta_key,
                             std::string *meta_value) {
  std::string value;
  Status s = zp_->Get(kZgwMetaTableName, meta_key, meta_value);
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

Status ZgwStore::DelBucket(const std::string &name) {
  return zp_->Delete(kZgwMetaTableName, ZgwBucket(name).MetaKey());
}

Status ZgwStore::ListBuckets(NameList *names, std::vector<ZgwBucket> *buckets) {
  // Get Bucket Meta
  Status s;
  std::string value;
  std::lock_guard<std::mutex> lock(names->list_lock);
  for (auto& name : names->name_list) {
    ZgwBucket obucket(name);
    zp_->Get(kZgwMetaTableName, obucket.MetaKey(), &value);
    if (!obucket.ParseMetaValue(value).ok()) {
      continue; // Skip table with not meta info
    }
    buckets->push_back(obucket);
  }

  return Status::OK();
}

Status ZgwStore::ListObjects(const std::string &bucket_name, NameList *names,
                             std::vector<ZgwObject> *objects, bool list_multiupload) {
  Status s;
  std::string meta_value;
  std::lock_guard<std::mutex> lock(names->list_lock);
  for (auto &object_name : names->name_list) {
    if (list_multiupload && (object_name.find("__") != 0)) {  // skip normal object
      continue;
    }
    ZgwObject ob(object_name);
    s = zp_->Get(kZgwMetaTableName, bucket_name + ob.MetaKey(), &meta_value);
    if (!s.ok()) {
      return s;
    }
    ob.ParseMetaValue(&meta_value);
    if (list_multiupload) {
      if (!ob.multiparts_done()) {
        objects->push_back(ob);
      }
    } else {
      if (ob.multiparts_done()) {
        objects->push_back(ob);
      }
    }
  }
  return Status::OK();
}

}  // namespace libzgw
