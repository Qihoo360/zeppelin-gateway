#ifndef ZGW_STORE_H
#define ZGW_STORE_H
#include <string>
#include <vector>

#include "slash_status.h"
#include "zp_cluster.h"
#include "zgw_bucket.h"
#include "zgw_object.h"
#include "zgw_user.h"

using slash::Status;

namespace libzgw {

class ZgwStore {
public:
  static Status Open(const std::vector<std::string>& ips, ZgwStore** ptr);
  ~ZgwStore();

  Status LoadAllUsers();
  Status AddUser(const std::string &user_name,
                 std::string *access_key,
                 std::string *secret_key);
  Status GetUser(const std::string &access_key, ZgwUser **user);
  
  Status AddBucket(const std::string &access_key,
                   const std::string &bucket_name, int partition_num = 10);
  Status ListBuckets(const std::string &access_key,
                     std::vector<ZgwBucket>* buckets);
  Status DelBucket(const std::string &access_key, const std::string &bucket_name);
  
  Status AddObject(const std::string &access_key,
                   const std::string &bucket_name,
                   const std::string &object_name,
                   const ZgwObjectInfo& info,
                   const std::string &content);
  Status GetObject(const std::string &access_key,
                   const std::string &bucket_name,
                   const std::string& object_name,
                   ZgwObject* object);
  Status DelObject(const std::string &access_key,
                   const std::string &bucket_name,
                   const std::string &object_name);
  Status ListObjects(const std::string &access_key,
                     const std::string &bucket_name,
                     std::vector<ZgwObject>* objects);

private:
  ZgwStore();
  Status Init(const std::vector<std::string>& ips);
  libzp::Cluster* zp_;
  ZgwUserList user_list_;
  std::map<std::string, ZgwUser*> access_key_user_map_;

  Status BuildMap();
  std::string GetRandomKey(int width);
};


}  // namespace libzgw

#endif
