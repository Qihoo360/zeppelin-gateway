#ifndef ZGW_STORE_H_
#define ZGW_STORE_H_

#include "slash/include/slash_status.h"
#include "libzp/include/zp_cluster.h"
#include "hiredis.h"
#include "zgw_define.h"

using slash::Status;

namespace zgwstore {

class ZgwStore {
 public:
  ZgwStore(const std::string& zp_table, const std::string& lock_name,
      const int32_t lock_ttl, const std::string& redis_passwd);
  ~ZgwStore();
  static Status Open(const std::vector<std::string>& zp_addrs,
      const std::string& redis_addr, const std::string& zp_table,
      const std::string& lock_name, const int32_t lock_ttl,
      const std::string& redis_passwd, ZgwStore** store);
  void set_redis_ip(const std::string& redis_ip) {
    redis_ip_ = redis_ip;
  }
  void set_redis_port(const int32_t redis_port) {
    redis_port_ = redis_port;
  }
  void InstallClients(libzp::Cluster* zp_cli, redisContext* redis_cli);

  Status BlockSet(const std::string& block_id, const std::string& block_content);
  Status BlockGet(const std::string& block_id, std::string* block_content);
  Status BlockDelete(const std::string& block_id);
  Status BlockMGet(const std::vector<std::string>& block_ids,
      std::map<std::string, std::string>* block_contents);

  Status Lock();
  Status UnLock();

  Status AddUser(const User& user, const bool override = false);
  Status ListUsers(std::vector<User>* users);

  Status AddBucket(const Bucket& bucket, const bool need_lock = true,
      const bool override = false);
  Status GetBucket(const std::string& user_name, const std::string& bucket_name,
      Bucket* bucket);
  Status DeleteBucket(const std::string& user_name, const std::string& bucket_name,
      const bool need_lock = true);
  Status ListBuckets(const std::string& user_name, std::vector<Bucket>* buckets);

  Status AllocateId(const std::string& user_name, const std::string& bucket_name,
      const std::string& object_name, const int32_t block_nums, uint64_t* tail_id);
  Status AddObject(const Object& object, const bool need_lock = true);
  Status GetObject(const std::string& user_name, const std::string& bucket_name,
      const std::string& object_name, Object* object);
  Status DeleteObject(const std::string& user_name, const std::string& bucket_name,
      const std::string& object_name, const bool need_lock = true);
  Status ListObjects(const std::string& user_name, const std::string& bucket_name,
      std::vector<Object>* objects);

  Status AddMultiBlockSet(const std::string& bucket_name, const std::string& object_name,
      const std::string& upload_id, const std::string& block_index);
  Status GetMultiBlockSet(const std::string& bucket_name, const std::string& object_name,
      const std::string& upload_id, std::vector<std::string>* block_indexs);
  Status DeleteMultiBlockSet(const std::string& bucket_name, const std::string& object_name,
      const std::string& upload_id);
 private:
  bool MaybeHandleRedisError();
  Status HandleIOError(const std::string& func_name);
  Status HandleLogicError(const std::string& str_err, redisReply* reply,
      const bool should_unlock);
  bool CheckRedis();

  User GenUserFromReply(redisReply* reply);
  Bucket GenBucketFromReply(redisReply* reply);
  Object GenObjectFromReply(redisReply* reply);

  std::string zp_table_;
  libzp::Cluster* zp_cli_;
  redisContext* redis_cli_;
  std::string redis_ip_;
  int32_t redis_port_;
  std::string lock_name_;
  int32_t lock_ttl_;
  std::string redis_passwd_;
  bool redis_error_;
};

}
#endif

