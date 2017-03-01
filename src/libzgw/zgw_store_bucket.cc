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

  // // Check whether exist
  // auto &buckets_name = user->buckets_name();
  // if (buckets_name.find(bucket.name()) != buckets_name.end()) {
  //   return Status::NotSupported(bucket.name() + " Already Created");
  // }

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
    sleep(2); // waiting zeppelin
    s = zp_->Set(bucket.name(), bucket.MetaKey(), bucket.MetaValue());
  } while (retry-- && s.IsNotSupported());
  if (!s.ok()) {
    return s;
  }

  // // Add bucket meta to user info
  // buckets_name.insert(bucket_name);
  // // Dump user info
  // s = zp_->Set(kUserTableName, user->MetaKey(), user->MetaValue());
  // if (!s.ok()) {
  //   return s;
  // }

  return s;
}

Status ZgwStore::SaveNameList(std::string &access_key, NameList *name_list) {
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

  s = zp_->Set(name_list->table_name(), name_list->MetaKey(), name_list->MetaValue());
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

Status ZgwStore::GetNameList(std::string &access_key, NameList *name_list) {
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
  s = zp_->Get(name_list->table_name(), name_list->MetaKey(), &value);
  if (!s.ok()) {
    return s;
  }

  return name_list->ParseMetaValue(&value);
}

#if 0
Status ZgwStore::ListBuckets(const std::string &access_key,
                             std::vector<ZgwBucket>* buckets) {
  assert(buckets);
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

  // Get user's buckets name
  auto &buckets_name = user->buckets_name();
  
  // Get Bucket Meta
  std::string value;
  for (auto& name : buckets_name) {
    ZgwBucket obucket(name);
    zp_->Get(name, obucket.MetaKey(), &value);
    if (!obucket.ParseMetaValue(value).ok()) {
      continue; // Skip table with not meta info
    }
    buckets->push_back(obucket);
  }

  return Status::OK();
}
#endif  // #if 0

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

  // // Delete from user info
  // auto &buckets_name = user->buckets_name();
  // buckets_name.erase(name);

  // Check whether is empty bucket

  // TODO wangkang-xy Delete Bucket

  // // Dump user info
  // s = zp_->Set(kUserTableName, user->MetaKey(), user->MetaValue());
  // if (!s.ok()) {
  //   return s;
  // }
  
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
  std::string value;
  for (const auto& name : names->name_list()) {
    ZgwBucket obucket(name);
    zp_->Get(name, obucket.MetaKey(), &value);
    if (!obucket.ParseMetaValue(value).ok()) {
      continue; // Skip table with not meta info
    }
    buckets->push_back(obucket);
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

  std::string meta_value;
  for (auto &object_name : names->name_list()) {
    ZgwObject ob(object_name);
    s = zp_->Get(bucket_name, ob.MetaKey(), &meta_value);
    if (!s.ok()) {
      return s;
    }
    ob.ParseMetaValue(&meta_value);
    objects->push_back(ob);
  }

  return Status::OK();
}

#if 0
Status ZgwStore::ListObjects(const std::string &access_key,
                             const std::string &bucket_name,
                             std::vector<ZgwObject>* objects) {
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

  std::string meta_value;
  ZgwBucket bucket(bucket_name);
  s = zp_->Get(bucket_name, bucket.MetaKey(), &meta_value);
  if (!s.ok()) {
    return s;
  }
  bucket.ParseMetaValue(meta_value);

  meta_value.clear();
  for (auto &object_name : bucket.objects_name()) {
    ZgwObject ob(object_name);
    s = zp_->Get(bucket_name, ob.MetaKey(), &meta_value);
    if (!s.ok()) {
      return s;
    }
    ob.ParseMetaValue(&meta_value);
    objects->push_back(ob);
  }

  return Status::OK();
}
#endif  // #if 0

}  // namespace libzgw
