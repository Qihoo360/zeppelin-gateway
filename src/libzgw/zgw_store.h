#ifndef ZGW_STORE_H
#define ZGW_STORE_H
#include <string>
#include <vector>

#include "slash_status.h"
#include "zp_cluster.h"
#include "zgw_bucket.h"
#include "zgw_object.h"
#include "zgw_user.h"
#include "zgw_namelist.h"

using slash::Status;

namespace libzgw {

class NameList;
class ZgwObjectInfo;
class ZgwObject;

class ZgwStore {
public:
  static Status Open(const std::vector<std::string>& ips, ZgwStore** ptr);
  ~ZgwStore();

  // Operation On Service
  Status LoadAllUsers();
  Status AddUser(const std::string &user_name,
                 std::string *access_key, std::string *secret_key);
  Status GetUser(const std::string &access_key, ZgwUser **user);
  Status ListUsers(std::set<ZgwUser *> *user_list);
  Status SaveNameList(const std::string &table_name, const std::string &meta_key,
                      const std::string &meta_value);
  Status GetNameList(const std::string &table_name, const std::string &meta_key,
                     std::string *meta_value);
  
  // Operation On Buckets
  Status AddBucket(const std::string &bucket_name, ZgwUserInfo user_info,
                   int partition_num = 3);
  Status ListBuckets(NameList *names, std::vector<ZgwBucket> *buckets);
  Status DelBucket(const std::string &bucket_name);
  Status ListObjects(const std::string &bucket_name, NameList *names,
                     std::vector<ZgwObject> *objects);
  
  // Operation On Objects
  Status AddObject(const std::string &bucket_name, const std::string &object_name,
                   const ZgwObjectInfo& info, const std::string &content);
  Status GetObject(const std::string &bucket_name, const std::string& object_name,
                   ZgwObject* object);
  Status DelObject(const std::string &bucket_name, const std::string &object_name);

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
