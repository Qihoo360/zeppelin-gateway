#include "zgw_store.h"

namespace libzgw {
  
  ZgwStore::ZgwStore() {
  }
  
  ZgwStore::~ZgwStore() {
  }

  Status ZgwStore::AddBucket(const std::string name, const ZgwBucket& bucket) {
    return Status::OK();
  }

  Status ZgwStore::ListBucket(std::vector<std::string>* buckets) {
    return Status::OK();
  }


}
