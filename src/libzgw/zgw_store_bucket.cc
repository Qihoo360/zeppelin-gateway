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
    s = zp_->Set(bucket.name(), bucket.MetaKey(), bucket.MetaValue());
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
      zp_->Get(name, obucket.MetaKey(), &value);
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
  s = zp_->Get(name, ZgwBucket(name).MetaKey(), &tvalue);
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

  return Status::OK();
}

}
