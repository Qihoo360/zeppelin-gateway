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

Status ZgwStore::GetBucket(ZgwBucket *bucket) {
  assert(bucket);
  std::string meta_value;
  Status s = zp_->Get(kZgwMetaTableName, bucket->MetaKey(), &meta_value);
  if (!s.ok()) {
    return s;
  }
  return bucket->ParseMetaValue(meta_value);
}

}  // namespace libzgw
