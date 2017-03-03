#include <unistd.h>

#include "zgw_store.h"
#include "zgw_user.h"

#include "include/slash_string.h"

namespace libzgw {
 
Status ZgwStore::AddBucket(const std::string &access_key,
                           const std::string &bucket_name,
                           int partition_num) {
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

  ZgwBucket bucket(bucket_name);

  // Create Bucket
  s = zp_->CreateTable(bucket.name(), partition_num);
  if (!s.ok() && !s.IsNotSupported()) {
    return s;
  } else {
    // Alread create, but not found in user meta, continue
  }

  // Add Bucket Meta
  int retry = 3;
  bucket.SetUserInfo(user->user_info());
  do {
    sleep(2); // waiting zeppelin create partitions
    s = zp_->Set(bucket.name(), bucket.MetaKey(), bucket.MetaValue());
    if (s.ok()) {
      break;
    }
  } while (retry-- && s.IsNotSupported());

  return s;
}

Status ZgwStore::SaveNameList(const std::string &access_key,
                              const std::string &table_name,
                              const std::string &meta_key,
                              const std::string &meta_value) {
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

  s = zp_->Set(table_name, meta_key, meta_value);
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

Status ZgwStore::GetNameList(const std::string &access_key,
                             const std::string &table_name,
                             const std::string &meta_key,
                             std::string *meta_value) {
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

  std::string value;
  s = zp_->Get(table_name, meta_key, meta_value);
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

Status ZgwStore::DelBucket(const std::string &access_key,
                           const std::string &name) {
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

  // Check whether is empty bucket

  // TODO wangkang-xy Delete Bucket

  return Status::OK();
}

Status ZgwStore::ListBuckets(const std::string &access_key,
                             NameList *names,
                             std::vector<ZgwBucket> *buckets) {
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

  // Get Bucket Meta
  {
    std::string value;
    std::lock_guard<std::mutex> lock(names->list_lock);
    for (auto& name : names->name_list) {
      ZgwBucket obucket(name);
      zp_->Get(name, obucket.MetaKey(), &value);
      if (!obucket.ParseMetaValue(value).ok()) {
        continue; // Skip table with not meta info
      }
      buckets->push_back(obucket);
    }
  }

  return Status::OK();
}

Status ZgwStore::ListObjects(const std::string &access_key,
                             const std::string &bucket_name,
                             NameList *names,
                             std::vector<ZgwObject> *objects) {
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

  {
    std::string meta_value;
    std::lock_guard<std::mutex> lock(names->list_lock);
    for (auto &object_name : names->name_list) {
      ZgwObject ob(object_name);
      s = zp_->Get(bucket_name, ob.MetaKey(), &meta_value);
      if (!s.ok()) {
        return s;
      }
      ob.ParseMetaValue(&meta_value);
      objects->push_back(ob);
    }
  }

  return Status::OK();
}

}  // namespace libzgw
