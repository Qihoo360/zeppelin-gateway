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
  ZgwStore(const std::string& lock_name, const int32_t lock_ttl);
  ~ZgwStore();
  static Status Open(const std::vector<std::string>& zp_addrs,
      const std::string& redis_addr, const std::string& lock_name,
      const int32_t lock_ttl, ZgwStore** store);
  void set_redis_ip(const std::string& redis_ip) {
    redis_ip_ = redis_ip;
  }
  void set_redis_port(const int32_t redis_port) {
    redis_port_ = redis_port;
  }
  void InstallClients(libzp::Cluster* zp_cli, redisContext* redis_cli);
  Status AddUser(const User& user, const bool override = false);
  Status ListUsers(std::vector<User>* users);

  Status AddBucket(const Bucket& bucket, const bool override = false);
  Status ListBuckets(const std::string& user_name, std::vector<Bucket>* buckets);
 private:
  bool MaybeHandleRedisError();
  Status HandleIOError(const std::string& func_name);
  Status HandleLogicError(const std::string& str_err, redisReply* reply,
      const bool should_unlock);

  User GenUserFromReply(redisReply* reply);
  Bucket GenBucketFromReply(redisReply* reply);
  Status Lock();
  Status UnLock();

  libzp::Cluster* zp_cli_;
  redisContext* redis_cli_;
  std::string redis_ip_;
  int32_t redis_port_;
  std::string lock_name_;
  int32_t lock_ttl_;
  bool redis_error_;
};

}
#endif

