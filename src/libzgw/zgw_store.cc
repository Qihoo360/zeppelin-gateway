#include "zgw_store.h"

namespace libzgw {
  
  ZgwStore::ZgwStore() {
  }
  
  ZgwStore::~ZgwStore() {
  }

  Status ZgwStore::AddBucket(const std::string name, const ZgwBucket& bucket) {
    return Status::OK();
  }

  Status ZgwStore::ListBucket(std::vector<ZgwBucket>* buckets) {
    buckets->push_back(ZgwBucket("test1", "2017-02-15T03:29:14.610Z"));
    buckets->push_back(ZgwBucket("test2", "2017-02-15T06:34:25.132Z"));
    return Status::OK();
  }


}
