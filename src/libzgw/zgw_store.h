#ifndef ZGW_STORE_H
#define ZGW_STORE_H
#include <string>
#include <vector>

#include "slash_status.h"
#include "zp_cluster.h"
#include "zgw_bucket.h"

using slash::Status;

namespace libzgw {

class ZgwStore {
public:
  ZgwStore();
  ~ZgwStore();
  
  Status AddBucket(const ZgwBucket& bucket);
  Status ListBuckets(std::vector<ZgwBucket>* buckets);
  Status DelBucket(const std::string &bucket_name);

  Status DelObject(const std::string &bucket_name,
                   const std::string &object_name);
  Status ListObjects(const std::string &bucket_name,
                     std::vector<ZgwObject>* objects);

private:
  libzp::Cluster* zp_;

};


}

#endif
