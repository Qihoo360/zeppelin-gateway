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

static const std::string kZgwMetaTableName = "__zgw_meta_table";
static const std::string kZgwDataTableName = "__zgw_data_table";
static const int kZgwTablePartitionNum = 10;
 
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
  Status SaveNameList(const std::string &meta_key, const std::string &meta_value);
  Status GetNameList(const std::string &meta_key, std::string *meta_value);
  
  // Operation On Buckets
  Status AddBucket(const std::string& bucket_name, const ZgwUserInfo& user_info);
  Status GetBucket(ZgwBucket *bucket);
  Status DelBucket(const std::string &bucket_name);
  
  // Operation On Objects
  Status AddObject(const std::string &bucket_name, const std::string &object_name,
                   const ZgwObjectInfo& info, const std::string &content);
  Status GetObject(ZgwObject* object, bool need_content = false);
  Status DelObject(const std::string &bucket_name, const std::string &object_name);
  Status InitMultiUpload(const std::string &bucket_name, const std::string &object_name,
                         std::string *upload_id, std::string *internal_obname,
                         ZgwUser *initiator);
  Status UploadPart(const std::string& bucket_name, const std::string& internal_obname,
                    const ZgwObjectInfo& info, const std::string& content, int part_num);
  Status ListParts(const std::string& bucket_name, const std::string& internal_obname,
                   std::vector<ZgwObject> *parts);
  Status CompleteMultiUpload(const std::string& bucket_name, const std::string& internal_obname);

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
