#include "zgw_store.h"

#include "iostream"

namespace libzgw {
  
  ZgwStore::ZgwStore() {
  }
  
  ZgwStore::~ZgwStore() {
  }

  Status ZgwStore::AddBucket(const ZgwBucket& bucket) {
    std::cout << "Create Bucket OK" << std::endl;
    return Status::OK();
  }

  Status ZgwStore::ListBuckets(std::vector<ZgwBucket>* buckets) {
    buckets->push_back(ZgwBucket("test1", "2017-02-15T03:29:14.610Z"));
    buckets->push_back(ZgwBucket("test2", "2017-02-15T06:34:25.132Z"));
    return Status::OK();
  }

  Status ZgwStore::DelBucket(const std::string &bucket_name) {
    std::cout << "Delete Bucket OK" << std::endl;
    return Status::OK();
  }

  Status ZgwStore::DelObject(const std::string &bucket_name,
                             const std::string &object_name) {
    std::cout << "Delete Object OK" << std::endl;
    return Status::OK();
  }

  Status ZgwStore::ListObjects(const std::string &bucket_name,
                               std::vector<ZgwObject>* buckets) {
    buckets->push_back(ZgwObject(bucket_name, "key1"));
    buckets->push_back(ZgwObject(bucket_name, "key2"));
    return Status::OK();
  }
}
