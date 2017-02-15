#ifndef ZGW_STORE_H
#define ZGW_STORE_H
#include <string>
#include <vector>

#include "slash"
#include "zp_cluster.h"
#include "zgw_bucket.h"

using slash::Status;

namespace libzgw {

class ZgwStore {
public:
  ZgwStore();
  ~ZgwStore();
  
  Status AddBucket(const std::string name, const ZgwBucket& bucket);
  Status ListBucket(std::vector<std::string>* buckets);
  Status GetAllObjects(const std::string name, std::vector<ZgwObject*>* objects);


private:
  libzp::Cluster* zp_;

};


}

#endif
