#ifndef ZGW_STORE_H
#define ZGW_STORE_H
#include <string>
#include <vector>

#include "slash_status.h"
#include "zp_cluster.h"
#include "zgw_bucket.h"
#include "zgw_object.h"

using slash::Status;

namespace libzgw {

class ZgwStore {
public:
  Status Open(const std::string& name,
    const std::vector<std::string>& ips, ZgwStore** ptr);

  ~ZgwStore();
  
  Status AddBucket(const ZgwBucket& bucket, int partition_num = 1024);
  Status ListBuckets(std::vector<ZgwBucket>* buckets);
  Status DelBucket(const std::string &bucket_name);
  
  Status AddObject(const std::string &bucket_name,
      const ZgwObject& object);
  Status DelObject(const std::string &bucket_name,
      const std::string &object_name);
  Status ListObjects(const std::string &bucket_name,
      std::vector<ZgwObject>* objects);

private:
  ZgwStore(const std::string& name);
  Status Init(const std::vector<std::string>& ips);
  std::string name_;
  libzp::Cluster* zp_;

};


}

#endif
