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
  Status Open(const std::string& name,
    const std::vector<std::string>& ips, ZgwStore** ptr);

  ~ZgwStore();
  
  Status AddBucket(const ZgwBucket& bucket, int partition_num = 1024);
  Status ListBucket(std::vector<ZgwBucket*>* buckets);


private:
  ZgwStore(const std::string& name);
  Status Init(const std::vector<std::string>& ips);
  std::string name_;
  libzp::Cluster* zp_;

};


}

#endif
