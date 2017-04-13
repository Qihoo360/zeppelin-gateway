#include "zgw_store.h"

#include <unistd.h>

#include "slash/include/slash_string.h"
#include "zgw_user.h"

namespace libzgw {
 
Status ZgwStore::AddBucket(const std::string& bucket_name, const ZgwUserInfo& user_info) {
  // Add Bucket Meta
  ZgwBucket bucket(bucket_name);
  bucket.SetUserInfo(user_info);
  return zp_->Set(kZgwMetaTableName, bucket.MetaKey(), bucket.MetaValue());
}

Status ZgwStore::SaveNameList(const std::string &meta_key,
                              const std::string &meta_value) {
  return zp_->Set(kZgwMetaTableName, meta_key, meta_value);
}

Status ZgwStore::GetNameList(const std::string &meta_key,
                             std::string *meta_value) {
  return zp_->Get(kZgwMetaTableName, meta_key, meta_value);
}

Status ZgwStore::DelBucket(const std::string &name) {
  return zp_->Delete(kZgwMetaTableName, ZgwBucket(name).MetaKey());
}

Status ZgwStore::GetBucket(ZgwBucket* bucket) {
  assert(bucket);
  Status s;
  std::string meta_value;

  s = zp_->Get(kZgwMetaTableName, bucket->MetaKey(), &meta_value);
  if (!s.ok()) {
    return s;
  }

  return bucket->ParseMetaValue(meta_value);
}

Status ZgwStore::ListBucket(const std::set<std::string>& name_list,
                            std::vector<ZgwBucket>* buckets) {
  assert(buckets);
  Status s;
  std::vector<std::string> keys;
  std::map<std::string, std::string> values;
  for (auto& name : name_list) {
    ZgwBucket b(name);
    keys.push_back(b.MetaKey());
    buckets->push_back(std::move(b));
  }
  s = zp_->Mget(kZgwMetaTableName, keys, &values);
  if (!s.ok()) {
    return s;
  }

  for (auto& b : *buckets) {
    s = b.ParseMetaValue(values[b.MetaKey()]);
    if (!s.ok()) {
      return s;
    }
  }
  return Status::OK();
}

}  // namespace libzgw
