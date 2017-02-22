#include <unistd.h>

#include "zgw_store.h"

#include "include/slash_string.h"
#include <glog/logging.h>

namespace libzgw {
 
Status ZgwStore::AddBucket(const ZgwBucket& bucket,
    int partition_num) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Create Bucket
  s = zp_->CreateTable(bucket.name(), partition_num);
  if (!s.ok()) {
    return s;
  }

  // Add Bucket Meta
  int retry = 3;
  do {
    sleep(2); // waiting zeppelin
    s = zp_->Set(bucket.name(), ZgwBucket::MetaKey(bucket.name()), bucket.MetaValue());
  } while (retry-- && s.IsNotSupported());
  return s;
}

Status ZgwStore::ListBuckets(std::vector<ZgwBucket>* buckets) {
  assert(buckets);
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Get all Bucket name 
  std::vector<std::string> bucket_names;
  s = zp_->ListTable(&bucket_names);
  if (!s.ok()) {
    return s;
  }
  
  // Get Bucket Meta
  if (!bucket_names.empty()) {
    for (std::string& name : bucket_names) {
      std::string value;
      ZgwBucket obucket(name);
      zp_->Get(name, ZgwBucket::MetaKey(name), &value);
      if (!obucket.ParseMetaValue(value).ok()) {
        continue; // Skip table with not meta info
      }
      buckets->push_back(obucket);
    }
  }

  return Status::OK();
}


Status ZgwStore::DelBucket(const std::string &name) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Check wether is bucket
  std::string tvalue;
  s = zp_->Get(name, ZgwBucket::MetaKey(name), &tvalue);
  if (s.IsNotFound()) {
    return Status::NotFound("bucket not found");
  } else {
    return s;
  }

  // TODO wangkang-xy Delete Bucket
  
  return Status::OK();
}

Status ZgwStore::ListObjects(const std::string &bucket_name,
                             std::vector<ZgwObject>* objects) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Get object list meta
  std::string meta_value;
  s = zp_->Get(bucket_name, ZgwBucket::MetaKey(bucket_name), &meta_value);
  if (!s.ok()) {
    return s;
  }

  // Bucket meta not used here
  uint32_t tmp;
  slash::GetFixed32(&meta_value, &tmp);
  slash::GetFixed32(&meta_value, &tmp);

  // Parse objects count;
  uint32_t name_count;
  std::string object_name;
  std::string ob_meta_value;
  slash::GetFixed32(&meta_value, &name_count);
  for (int i = 0; i < name_count; i++) {
    slash::GetLengthPrefixedString(&meta_value, &object_name);
    ZgwObject ob(object_name);
    s = zp_->Get(bucket_name, ob.MetaKey(), &ob_meta_value);
    if (!s.ok()) {
      return s;
    }
    s = ob.ParseMetaValue(&ob_meta_value);
    if (!s.ok()) {
      return s;
    }
    objects->push_back(ob);
  }

  return Status::OK();
}

}  // namespace libzgw
