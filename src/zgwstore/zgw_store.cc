#include "zgw_store.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <glog/logging.h>
#include "slash/include/slash_string.h"
#include "slash/include/env.h"

namespace zgwstore {

ZgwStore::ZgwStore(const std::string& zp_table,const std::string& lock_name,
                   const int32_t lock_ttl, const std::string& redis_passwd)
      : zp_table_(zp_table),
        zp_cli_(nullptr),
        redis_cli_(nullptr),
        redis_ip_(""),
        redis_port_(-1),
        lock_name_(lock_name),
        lock_ttl_(lock_ttl),
        redis_passwd_(redis_passwd),
        redis_error_(false) {
};

ZgwStore::~ZgwStore() {
  if (zp_cli_ != nullptr) {
    delete zp_cli_;
  }
  if (redis_cli_ != nullptr) {
    redisFree(redis_cli_);
  }
}

Status ZgwStore::Open(
    const std::vector<std::string>& zp_addrs, const std::string& zp_table,
    int zp_op_timeout_ms, const std::string& redis_addr,
    const std::string& lock_name, const int32_t lock_ttl,
    const std::string& redis_passwd, ZgwStore** store) {

  *store = nullptr;
  /*
   * Connect to zeppelin
   */
  if (zp_addrs.empty()) {
    return Status::InvalidArgument("Invalid zeppelin addresses");
  }

  std::string t_ip;
  int t_port = 0;
  Status s;
  libzp::Options zp_option;
  for (auto& addr : zp_addrs) {
    if (!slash::ParseIpPortString(addr, t_ip, t_port)) {
      return Status::InvalidArgument("Invalid zeppelin address");
    }
    zp_option.meta_addr.push_back(libzp::Node(t_ip, t_port));
  }
  zp_option.op_timeout = zp_op_timeout_ms;
  libzp::Cluster* zp_cli = new libzp::Cluster(zp_option);
  s = zp_cli->Connect();
  if (!s.ok()) {
    delete zp_cli;
    return Status::IOError("Failed to connect to zeppelin");
  }
  /*
   *  Connect to redis
   */
  if (!slash::ParseIpPortString(redis_addr, t_ip, t_port)) {
    delete zp_cli;
    return Status::InvalidArgument("Invalid zeppelin address");
  }
  redisContext* redis_cli;

  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  redis_cli = redisConnectWithTimeout(t_ip.c_str(), t_port, timeout);
  if (redis_cli == NULL || redis_cli->err) {
    delete zp_cli;
    if (redis_cli) {
      redisFree(redis_cli);
      return Status::IOError("Failed to connect to redis");
    } else {
      return Status::Corruption("Connection error: can't allocate redis context");
    }
  }
  if (!redis_passwd.empty()) {
    redisReply* reply = static_cast<redisReply*>(redisCommand(redis_cli,
                "AUTH %s", redis_passwd.c_str()));
    if (reply == NULL) {
      delete zp_cli;
      return Status::IOError("Failed to auth to redis");
    }
    if (std::string(reply->str) != "OK") {
      freeReplyObject(reply);
      delete zp_cli;
      return Status::Corruption("Failed to auth to redis");
    }
    freeReplyObject(reply);
  }

  *store = new ZgwStore(zp_table, lock_name, lock_ttl, redis_passwd);
  (*store)->InstallClients(zp_cli, redis_cli);
  (*store)->set_redis_ip(t_ip);
  (*store)->set_redis_port(t_port);
  return Status::OK();
}

void ZgwStore::InstallClients(libzp::Cluster* zp_cli, redisContext* redis_cli) {
  zp_cli_ = zp_cli;
  redis_cli_ = redis_cli;
}

Status ZgwStore::BlockSet(const std::string& block_id, const std::string& block_content) {
  return zp_cli_->Set(zp_table_, kZpBlockPrefix + block_id, block_content);
}

Status ZgwStore::BlockGet(const std::string& block_id, std::string* block_content) {
  return zp_cli_->Get(zp_table_, kZpBlockPrefix + block_id, block_content);
}

Status ZgwStore::BlockMGet(const std::vector<std::string>& block_ids,
    std::map<std::string, std::string>* block_contents) {
  std::vector<std::string> ids;
  for(auto& id : block_ids) {
    ids.push_back(kZpBlockPrefix + id);
  }
  return zp_cli_->Mget(zp_table_, ids, block_contents);
}

Status ZgwStore::BlockRef(const std::string& block_id) {
  // Lock outside
  std::string block_ref_s;
  int ref;
  Status s = zp_cli_->Get(zp_table_, kZpRefPrefix + block_id, &block_ref_s);
  if (!s.ok()) {
    if (s.IsNotFound()) {
      ref = 0;
    } else {
      return s;
    }
  } else {
    ref = std::atoi(block_ref_s.c_str());
  }

  ref++;

  block_ref_s.assign(std::to_string(ref));
  return zp_cli_->Set(zp_table_, kZpRefPrefix + block_id, block_ref_s);
}

Status ZgwStore::BlockUnref(uint64_t block_id) {
  // Assert lock held
  std::string block_ref_s;
  bool should_delete = false;
  int ref;
  Status s = zp_cli_->Get(zp_table_, kZpRefPrefix + std::to_string(block_id),
                          &block_ref_s);
  if (s.ok()) {
    ref = std::atoi(block_ref_s.c_str());
    ref -= 1;
  } else if (s.IsNotFound()) {
    ref = -1;
  } else {
    return s;
  }

  if (ref < 0) {
    s = zp_cli_->Delete(zp_table_, kZpRefPrefix + std::to_string(block_id));
    if (!s.ok()) {
      return s;
    }
    return zp_cli_->Delete(zp_table_, kZpBlockPrefix + std::to_string(block_id));
  }

  return zp_cli_->Set(zp_table_, kZpRefPrefix + std::to_string(block_id),
                      std::to_string(ref));
}

Status ZgwStore::Lock() {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }

  redisReply *reply;
  while (true) {
    reply = static_cast<redisReply*>(redisCommand(redis_cli_,
                "SET zgw_lock %s NX PX %lu", lock_name_.c_str(), lock_ttl_));
    if (reply == NULL) {
      return HandleIOError("Lock");
    }
    if (reply->type == REDIS_REPLY_STATUS && !strcmp(reply->str, "OK")) {
      freeReplyObject(reply);
      break;
    }
    freeReplyObject(reply);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  return Status::OK();
}

Status ZgwStore::UnLock() {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }

  std::string del_cmd = "if redis.call(\"get\", \"zgw_lock\") == \"" + lock_name_ + "\" "
                        "then "
                        "return redis.call(\"del\", \"zgw_lock\") "
                        "else "
                        "return 0 "
                        "end ";

  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "EVAL %s %d", del_cmd.c_str(), 0));
  if (reply == NULL) {
    return HandleIOError("UnLock");
  }
  if (reply->integer == 1) {
    // UnLock Success
  } else if (reply->integer == 0) {
    // The zgw_lock is held by other clients
    // std::cout << "reply-int" << std::endl;
    // redisReply* t_reply = static_cast<redisReply*>(redisCommand(redis_cli_, "GET zgw_lock"));
    // std::cout <<t_reply->type << std::endl;
  }
  freeReplyObject(reply);
  return Status::OK();
}

Status ZgwStore::AddUser(const User& user, const bool override) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }

/*
 *  1. Lock
 */
  Status s;
  s = Lock();
  if (!s.ok()) {
    return s;
  }
/*
 *  2. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s %s", kZgwUserList.c_str(), user.display_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddUser::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddUser::SISMEMBER ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 1 && !override) {
    return HandleLogicError("User Already Exist", reply, true);
  }
  freeReplyObject(reply);
/*
 *  3. DEL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, "DEL %s%s",
              kZgwUserPrefix.c_str(), user.display_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddUser::DEL");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  4. HMSET
 */
  std::string hmset_cmd = "HMSET " + kZgwUserPrefix + user.display_name;
  hmset_cmd += (" uid " + user.user_id);
  hmset_cmd += (" name " + user.display_name);
  for (auto& iter : user.key_pairs) {
    hmset_cmd += (" " + iter.first + " " + iter.second);
  }
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, hmset_cmd.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddUser::HMSET");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddUser::HMSET ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_STATUS);
  freeReplyObject(reply);
/*
 *  4. SADD
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SADD %s %s", kZgwUserList.c_str(), user.display_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddUser::SADD");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddUser::SADD ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0 && !override) {
    return HandleLogicError("User Already Exist", reply, true);
  }
  freeReplyObject(reply);
/*
 *  5. UnLock
 */
  s = UnLock();
  return s;
}

Status ZgwStore::AddUserToken(const std::string& user_name,
                              const std::string& access_key,
                              const std::string& secret_key) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }

/*
 *  1. Lock
 */
  Status s;
  s = Lock();
  if (!s.ok()) {
    return s;
  }
/*
 *  2. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s %s", kZgwUserList.c_str(), user_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddUserToken::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddUserToken::SISMEMBER ret: " + std::string(reply->str),
                            reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("User NOT Exist", reply, true);
  }
  freeReplyObject(reply);
/*
 *  3. HSET
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "HSET %s%s %s %s",
              kZgwUserPrefix.c_str(), user_name.c_str(),
              access_key.c_str(), secret_key.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddUserToken::HSET");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddUserToken::HSET ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);

/*
 *  4. UnLock
 */
  s = UnLock();
  return s;
}

Status ZgwStore::DelUserToken(const std::string& user_name,
                              const std::string& access_key) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }

/*
 *  1. Lock
 */
  Status s;
  s = Lock();
  if (!s.ok()) {
    return s;
  }
/*
 *  2. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s %s", kZgwUserList.c_str(), user_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DelUserToken::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DelUserToken::SISMEMBER ret: " + std::string(reply->str),
                            reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("User NOT Exist", reply, true);
  }
  freeReplyObject(reply);

/*
 *  3. HDEL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "HDEL %s%s %s", kZgwUserPrefix.c_str(), user_name.c_str(),
              access_key.c_str()));
  if (reply == NULL) {
    return HandleIOError("DelUserToken::HDEL");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DelUserToken::HDEL ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);

/*
 *  4. UnLock
 */
  s = UnLock();
  return s;
}

Status ZgwStore::ListUsers(std::vector<User>* users) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
  users->clear();
/*
 *  1. Get user list
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SMEMBERS %s", kZgwUserList.c_str()));
  if (reply == NULL) {
    return HandleIOError("ListUsers::SEMBMBERS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("ListUser::SMEMBERS ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements == 0) {
    freeReplyObject(reply);
    return Status::OK();
  }
/*
 *  2. Iterate through users to HGETALL
 */
  redisReply* t_reply;
  for (unsigned int i = 0; i < reply->elements; i++) {
    t_reply = static_cast<redisReply*>(redisCommand(redis_cli_, "HGETALL %s%s",
          kZgwUserPrefix.c_str(), reply->element[i]->str));
    if (t_reply == NULL) {
      freeReplyObject(reply);
      return HandleIOError("ListUsers::HGETALL");
    }
    if (t_reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      return HandleLogicError("ListUser::HGETALL ret: " + std::string(t_reply->str), t_reply, false);
    }
    assert(t_reply->type == REDIS_REPLY_ARRAY);
    if (t_reply->elements == 0) {
      continue;
    } else if (t_reply->elements % 2 != 0) {
      freeReplyObject(reply);
      return HandleLogicError("ListUser::HGETALL: elements % 2 != 0", t_reply, false);
    }
    users->push_back(GenUserFromReply(t_reply));
    freeReplyObject(t_reply);
  }

  freeReplyObject(reply);
  return Status::OK();
}

Status ZgwStore::AddBucket(const Bucket& bucket, const bool need_lock,
    const bool override) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. Lock
 */
  Status s;
  if (need_lock) {
    s = Lock();
    if (!s.ok()) {
      return s;
    }
  }
/*
 *  2. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s%s %s", kZgwBucketListPrefix.c_str(), bucket.owner.c_str(),
              bucket.bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddBucket::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddBucket::SISMEMBER ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 1 && !override) {
    return HandleLogicError("Bucket Already Exist", reply, need_lock);
  }
  freeReplyObject(reply);
/*
 *  3. EXISTS
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, "EXISTS %s%s",
              kZgwBucketPrefix.c_str(), bucket.bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddBucket::EXISTS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddBucket::EXISTS ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 1) {
    return HandleLogicError("Bucket Already Exist [GLOBAL]", reply, need_lock);
  }
  assert(reply->integer == 0);
  freeReplyObject(reply);

/*
 *  4. DEL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, "DEL %s%s",
              kZgwBucketPrefix.c_str(), bucket.bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddBucket::DEL");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  5. HMSET
 */
  std::string hmset_cmd = "HMSET " + kZgwBucketPrefix + bucket.bucket_name;
  hmset_cmd += (" name " + bucket.bucket_name);
  hmset_cmd += " ctime %llu";
  hmset_cmd += (" owner " + bucket.owner);
  hmset_cmd += (" acl " + bucket.acl);
  hmset_cmd += (" loc " + bucket.location);
  hmset_cmd += " vol %lld";
  hmset_cmd += " uvol %lld";
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, hmset_cmd.c_str(), bucket.create_time,
        bucket.volumn, bucket.uploading_volumn));
  if (reply == NULL) {
    return HandleIOError("AddBucket::HMSET");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddBucket::HMSET ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_STATUS);
  freeReplyObject(reply);
/*
 *  6. SADD
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SADD %s%s %s", kZgwBucketListPrefix.c_str(), bucket.owner.c_str(),
              bucket.bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddBucket::SADD");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddBucket::SADD ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0 && !override) {
    return HandleLogicError("Bucket Already Exist", reply, need_lock);
  }
  freeReplyObject(reply);
/*
 *  7. UnLock
 */
  if (need_lock) {
    s = UnLock();
  }
  return s;
}

Status ZgwStore::GetBucket(const std::string& user_name, const std::string& bucket_name,
    Bucket* bucket) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s%s %s", kZgwBucketListPrefix.c_str(), user_name.c_str(),
              bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("GetBucket::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("GetBucket::SISMEMBER ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("Bucket Doesn't Belong To This User", reply, false);
  }
  freeReplyObject(reply);
/*
 *  HGETALL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, "HGETALL %s%s",
        kZgwBucketPrefix.c_str(), bucket_name.c_str()));
  if (reply == NULL) {
    freeReplyObject(reply);
    return HandleIOError("GetBucket::HGETALL");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    freeReplyObject(reply);
    return HandleLogicError("GetBucket::HGETALL ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements == 0) {
    return HandleLogicError("Bucket Not Found", reply, true);
  } else if (reply->elements % 2 != 0) {
    return HandleLogicError("GetBucket::HGETALL: elements % 2 != 0", reply, false);
  }
  *bucket = GenBucketFromReply(reply);
  freeReplyObject(reply);

  return Status::OK();
}

Status ZgwStore::DeleteBucket(const std::string& user_name, const std::string& bucket_name,
    const bool need_lock) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. Lock
 */
  Status s;
  if (need_lock) {
    s = Lock();
    if (!s.ok()) {
      return s;
    }
  }
/*
 *  2. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s%s %s", kZgwBucketListPrefix.c_str(), user_name.c_str(),
              bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteBucket::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DeleteBucket::SISMEMBER ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("Bucket Doesnt Exist", reply, need_lock);
  }
  freeReplyObject(reply);
/*
 *  3. HGET
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, "HGET %s%s vol",
              kZgwBucketPrefix.c_str(), bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteBucket::EXISTS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DeleteBucket::EXISTS ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_STRING);
  char* end;
  if (std::strtoll(reply->str, &end, 10) != 0) {
    return HandleLogicError("Bucket Vol IS NOT 0", reply, need_lock);
  }
  assert(reply->integer == 0);
  freeReplyObject(reply);
/*
 *  4. SCARD
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SCARD %s%s", kZgwObjectListPrefix.c_str(), bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteBucket::SCARD");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DeleteBucket::SCARD ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer != 0) {
    return HandleLogicError("Bucket Non Empty", reply, need_lock);
  }
  freeReplyObject(reply);
/*
 *  5. SREM
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SREM %s%s %s", kZgwBucketListPrefix.c_str(), user_name.c_str(),
              bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteObject::SREM");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  6. DEL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, "DEL %s%s",
              kZgwBucketPrefix.c_str(), bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteBucket::DEL");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  7. UnLock
 */
  if (need_lock) {
    s = UnLock();
  }
  return s;
}

Status ZgwStore::ListBuckets(const std::string& user_name, std::vector<Bucket>* buckets) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
  buckets->clear();
/*
 *  1. Get bucket list
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SMEMBERS %s%s", kZgwBucketListPrefix.c_str(),
              user_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("ListBuckets::SEMBMBERS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("ListBuckets::SMEMBERS ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements == 0) {
    freeReplyObject(reply);
    return Status::OK();
  }
/*
 *  2. Iterate through buckets to HGETALL
 */
  redisReply* t_reply;
  for (unsigned int i = 0; i < reply->elements; i++) {
    t_reply = static_cast<redisReply*>(redisCommand(redis_cli_, "HGETALL %s%s",
          kZgwBucketPrefix.c_str(), reply->element[i]->str));
    if (t_reply == NULL) {
      freeReplyObject(reply);
      return HandleIOError("ListBuckets::HGETALL");
    }
    if (t_reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      return HandleLogicError("ListBuckets::HGETALL ret: " + std::string(t_reply->str), t_reply, false);
    }
    assert(t_reply->type == REDIS_REPLY_ARRAY);
    if (t_reply->elements == 0) {
      continue;
    } else if (t_reply->elements % 2 != 0) {
      freeReplyObject(reply);
      return HandleLogicError("ListBuckets::HGETALL: elements % 2 != 0", t_reply, false);
    }
    buckets->push_back(GenBucketFromReply(t_reply));
    freeReplyObject(t_reply);
  }

  freeReplyObject(reply);
  return Status::OK();
}

Status ZgwStore::ListBucketsName(const std::string& user_name, std::vector<std::string>* buckets_name) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }

  buckets_name->clear();
/*
 *  1. Get bucket list
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SMEMBERS %s%s", kZgwBucketListPrefix.c_str(),
              user_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("ListBucketsName::SEMBMBERS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("ListBucketsName::SMEMBERS ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements == 0) {
    freeReplyObject(reply);
    return Status::OK();
  }
/*
 *  2. Iterate through buckets to push_back
 */
  for (unsigned int i = 0; i < reply->elements; i++) {
    buckets_name->push_back(reply->element[i]->str);
  }

  freeReplyObject(reply);
  return Status::OK();
}

Status ZgwStore::MGetBuckets(const std::string& user_name, const std::vector<std::string> buckets_name,
    std::vector<Bucket>* buckets) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
  buckets->clear();
/*
 *  1. Iterate through buckets to HGETALL
 */
  redisReply* t_reply;
  for (auto& bucket_name : buckets_name) {
    t_reply = static_cast<redisReply*>(redisCommand(redis_cli_, "HGETALL %s%s",
          kZgwBucketPrefix.c_str(), bucket_name.c_str()));
    if (t_reply == NULL) {
      return HandleIOError("MGetBuckets::HGETALL");
    }
    if (t_reply->type == REDIS_REPLY_ERROR) {
      return HandleLogicError("MGetBuckets::HGETALL ret: " + std::string(t_reply->str), t_reply, false);
    }
    assert(t_reply->type == REDIS_REPLY_ARRAY);
    if (t_reply->elements == 0) {
      continue;
    } else if (t_reply->elements % 2 != 0) {
      return HandleLogicError("MGetBuckets::HGETALL: elements % 2 != 0", t_reply, false);
    }
    buckets->push_back(GenBucketFromReply(t_reply));
    freeReplyObject(t_reply);
  }

  return Status::OK();
}

Status ZgwStore::AllocateId(const std::string& user_name, const std::string& bucket_name,
    const std::string& object_name, const int32_t block_nums, uint64_t* tail_id) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. Lock
 */
  Status s;
  s = Lock();
  if (!s.ok()) {
    return s;
  }
/*
 *  2. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s%s %s", kZgwBucketListPrefix.c_str(), user_name.c_str(),
              bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AllocateId::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AllocateId::SISMEMBER ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("Bucket Doesn't Belong To This User", reply, true);
  }
  freeReplyObject(reply);
/*
 *  3. EXISTS
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, "EXISTS %s%s",
              kZgwBucketPrefix.c_str(), bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AllocateId::EXISTS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AllocateId::EXISTS ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("Bucket NOT Exists", reply, true);
  }
  assert(reply->integer == 1);
/*
 *  4. SADD
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SADD %s%s %s%s", kZgwObjectListPrefix.c_str(), bucket_name.c_str(),
              kZgwTempObjectNamePrefix.c_str(), object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AllocateId::SADD");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AllocateId::SADD ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  5. INCRBY
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "INCRBY %s %d", kZgwIdGen.c_str(), block_nums));
  if (reply == NULL) {
    return HandleIOError("AllocateId::INCRBY");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AllocateId::INCRBY ret: " + std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  *tail_id = reply->integer;
  freeReplyObject(reply);
/*
 *  6. UnLock
 */
  s = UnLock();
  return s;
}

Status ZgwStore::AddObject(const Object& object, const bool need_lock) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. Lock
 */
  Status s;
  if (need_lock) {
    s = Lock();
    if (!s.ok()) {
      return s;
    }
  }
/*
 *  2. HGETALL
 */
  redisReply *reply;
  int64_t old_size = 0;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "HGETALL %s%s_%s", kZgwObjectPrefix.c_str(), object.bucket_name.c_str(),
              object.object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddObject::HGETALL");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddObject::HGETALL ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements != 0) {
    Object t_object = GenObjectFromReply(reply);
    old_size = t_object.size;
    redisReply* t_reply = static_cast<redisReply*>(redisCommand(redis_cli_,
                "LPUSH %s %s", kZgwDeletedList.c_str(), std::string(t_object.data_block +
                "/" + std::to_string(slash::NowMicros())).c_str()));
    if (t_reply == NULL) {
      return HandleIOError("AddObject::LPUSH");
    }
    if (t_reply->type == REDIS_REPLY_ERROR) {
      return HandleLogicError("AddObject::LPUSH ret: " + std::string(reply->str), reply, need_lock);
    }
    assert(t_reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(t_reply);
  }
  freeReplyObject(reply);
/*
 *  3. DEL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "DEL %s%s_%s", kZgwObjectPrefix.c_str(), object.bucket_name.c_str(),
              object.object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddObject::DEL");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  4. HMSET
 */
  std::string hmset_cmd = "HMSET " + kZgwObjectPrefix + object.bucket_name +
    "_" + object.object_name;
  hmset_cmd += (" bname " + object.bucket_name);
  hmset_cmd += (" oname " + object.object_name);
  hmset_cmd += (" etag " + object.etag);
  hmset_cmd += " size %lld";
  hmset_cmd += (" owner " + object.owner);
  hmset_cmd += " lm %llu";
  hmset_cmd += " class %d";
  hmset_cmd += (" acl " + object.acl);
  hmset_cmd += (" id " + object.upload_id);
  hmset_cmd += (" block " + object.data_block);
  reply = static_cast<redisReply*>(redisCommand(redis_cli_, hmset_cmd.c_str(), object.size,
        object.last_modified, object.storage_class));
  if (reply == NULL) {
    return HandleIOError("AddObject::HMSET");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddObject::HMSET ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_STATUS);
  freeReplyObject(reply);
/*
 *  5. SADD
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SADD %s%s %s", kZgwObjectListPrefix.c_str(), object.bucket_name.c_str(),
              object.object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddObject::SADD");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddObject::SADD ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    // object already exists, update
  }
  freeReplyObject(reply);
/*
 *  6. HINCRBY
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "HINCRBY %s%s vol %lld", kZgwBucketPrefix.c_str(), object.bucket_name.c_str(),
              object.size - old_size));
  if (reply == NULL) {
    return HandleIOError("AddObject::HINCRBY");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddObject::HINCRBY ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
/*
 *  7. SREM
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SREM %s%s %s%s", kZgwObjectListPrefix.c_str(), object.bucket_name.c_str(),
              kZgwTempObjectNamePrefix.c_str(), object.object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddObject::SREM");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  8. UnLock
 */
  if (need_lock) {
    s = UnLock();
  }
  return s;
}

Status ZgwStore::GetObject(const std::string& user_name, const std::string& bucket_name,
    const std::string& object_name, Object* object) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
  redisReply *reply;
/*
 *  1. SISMEMBER
 */
  if (!user_name.empty()) {
    reply = static_cast<redisReply*>(redisCommand(redis_cli_,
                                                  "SISMEMBER %s%s %s", kZgwBucketListPrefix.c_str(), user_name.c_str(),
                                                  bucket_name.c_str()));
    if (reply == NULL) {
      return HandleIOError("GetObject::SISMEMBER");
    }
    if (reply->type == REDIS_REPLY_ERROR) {
      return HandleLogicError("GetObject::SISMEMBER ret: " + std::string(reply->str), reply, false);
    }
    assert(reply->type == REDIS_REPLY_INTEGER);
    if (reply->integer == 0) {
      return HandleLogicError("Bucket Doesn't Belong To This User", reply, false);
    }
    freeReplyObject(reply);
  }
/*
 *  2. HGETALL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "HGETALL %s%s_%s", kZgwObjectPrefix.c_str(), bucket_name.c_str(),
              object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("GetObject::HGETALL");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("GetObject::HGETALL ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements == 0) {
    return HandleLogicError("Object Not Found", reply, true);
  } else if (reply->elements % 2 != 0) {
    return HandleLogicError("GetObject::HGETALL: elements % 2 != 0", reply, false);
  }
  *object = GenObjectFromReply(reply);
  freeReplyObject(reply);

  return Status::OK();
}

Status ZgwStore::DeleteObject(const std::string& user_name, const std::string& bucket_name,
    const std::string& object_name, const bool need_lock, const bool delete_block) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. Lock
 */
  Status s;
  if (need_lock) {
    s = Lock();
    if (!s.ok()) {
      return s;
    }
  }
/*
 *  2. HGETALL
 */
  redisReply *reply;
  int64_t delta_size = 0;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "HGETALL %s%s_%s", kZgwObjectPrefix.c_str(), bucket_name.c_str(),
              object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteObject::HGETALL");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DeleteObject::HGETALL ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements != 0) {
    Object t_object = GenObjectFromReply(reply);
    delta_size = t_object.size;
    if (delete_block) {
      redisReply* t_reply = static_cast<redisReply*>(redisCommand(redis_cli_,
          "LPUSH %s %s", kZgwDeletedList.c_str(), std::string(t_object.data_block +
          "/" + std::to_string(slash::NowMicros())).c_str()));
      if (t_reply == NULL) {
        freeReplyObject(reply);
        return HandleIOError("DeleteObject::LPUSH");
      }
      if (t_reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return HandleLogicError("DeleteObject::LPUSH ret: " + std::string(reply->str),
                                reply, need_lock);
      }
      assert(t_reply->type == REDIS_REPLY_INTEGER);
      freeReplyObject(t_reply);
    }
  }
  freeReplyObject(reply);
/*
 *  3. DEL
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "DEL %s%s_%s", kZgwObjectPrefix.c_str(), bucket_name.c_str(),
              object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteObject::DEL");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  4. SREM
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SREM %s%s %s", kZgwObjectListPrefix.c_str(), bucket_name.c_str(),
              object_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteObject::SREM");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DeleteObject::SREM ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
/*
 *  5. HINCRBY
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "HINCRBY %s%s vol %lld", kZgwBucketPrefix.c_str(), bucket_name.c_str(),
              0 - delta_size));
  if (reply == NULL) {
    return HandleIOError("DeleteObject::HINCRBY");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("DeleteObject::HINCRBY ret: " + std::string(reply->str), reply, need_lock);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
/*
 *  6. UnLock
 */
  if (need_lock) {
    s = UnLock();
  }
  return s;
}

Status ZgwStore::ListObjects(const std::string& user_name, const std::string& bucket_name,
    std::vector<Object>* objects) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s%s %s", kZgwBucketListPrefix.c_str(), user_name.c_str(),
              bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("ListObjects::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("ListObjects::SISMEMBER ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("Bucket Doesn't Belong To This User", reply, false);
  }
  freeReplyObject(reply);
/*
 *  2. Get object list
 */
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SMEMBERS %s%s", kZgwObjectListPrefix.c_str(),
              bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("ListObjects::SEMBMBERS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("ListObjects::SMEMBERS ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements == 0) {
    return Status::OK();
  }
/*
 *  3. Iterate through objects to HGETALL
 */
  redisReply* t_reply;
  for (unsigned int i = 0; i < reply->elements; i++) {
    t_reply = static_cast<redisReply*>(redisCommand(redis_cli_,
                "HGETALL %s%s_%s", kZgwObjectPrefix.c_str(), bucket_name.c_str(),
               reply->element[i]->str));
    if (t_reply == NULL) {
      freeReplyObject(reply);
      return HandleIOError("ListObjects::HGETALL");
    }
    if (t_reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      return HandleLogicError("ListObjects::HGETALL ret: " + std::string(t_reply->str), t_reply, false);
    }
    assert(t_reply->type == REDIS_REPLY_ARRAY);
    if (t_reply->elements == 0) {
      continue;
    } else if (t_reply->elements % 2 != 0) {
      freeReplyObject(reply);
      return HandleLogicError("ListObjects::HGETALL: elements % 2 != 0", t_reply, false);
    }
    objects->push_back(GenObjectFromReply(t_reply));
    freeReplyObject(t_reply);
  }
  freeReplyObject(reply);

  return Status::OK();
}

Status ZgwStore::ListObjectsName(const std::string& user_name, const std::string& bucket_name,
    std::vector<std::string>* objects_name) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
  objects_name->clear();
/*
 *  1. SISMEMBER
 */
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SISMEMBER %s%s %s", kZgwBucketListPrefix.c_str(), user_name.c_str(),
              bucket_name.c_str()));
  if (reply == NULL) {
    return HandleIOError("ListObjectsName::SISMEMBER");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("ListObjectsName::SISMEMBER ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("Bucket Doesn't Belong To This User", reply, false);
  }
  freeReplyObject(reply);
/*
 *  2. Get object list (SSCAN)
 */

  std::string cursor = "0";
  do {
    reply = static_cast<redisReply*>(redisCommand(redis_cli_,
                "SSCAN %s%s %s COUNT 1000", kZgwObjectListPrefix.c_str(),
                bucket_name.c_str(), cursor.c_str()));
    if (reply == NULL) {
      return HandleIOError("ListObjects::SSCAN");
    }
    if (reply->type == REDIS_REPLY_ERROR) {
      return HandleLogicError("ListObjects::SSCAN ret: " + std::string(reply->str), reply, false);
    }
    assert(reply->type == REDIS_REPLY_ARRAY);
    assert(reply->elements == 2);
    assert(reply->element[0]->type == REDIS_REPLY_STRING);
    assert(reply->element[1]->type == REDIS_REPLY_ARRAY);
    cursor = reply->element[0]->str;
    for (unsigned int i = 0; i < reply->element[1]->elements; i++) {
      objects_name->push_back(reply->element[1]->element[i]->str);
    }
    freeReplyObject(reply);

  } while (cursor != "0");

  return Status::OK();
}

Status ZgwStore::MGetObjects(const std::string& user_name, const std::string& bucket_name,
    const std::vector<std::string> objects_name, std::vector<Object>* objects) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
  objects->clear();
/*
 *  1. Iterate through objects to HGETALL
 */
  redisReply* t_reply;
  for (auto& object_name : objects_name) {
    t_reply = static_cast<redisReply*>(redisCommand(redis_cli_,
                "HGETALL %s%s_%s", kZgwObjectPrefix.c_str(), bucket_name.c_str(),
               object_name.c_str()));
    if (t_reply == NULL) {
      return HandleIOError("MGetObjects::HGETALL");
    }
    if (t_reply->type == REDIS_REPLY_ERROR) {
      return HandleLogicError("MGetObjects::HGETALL ret: " + std::string(t_reply->str), t_reply, false);
    }
    assert(t_reply->type == REDIS_REPLY_ARRAY);
    if (t_reply->elements == 0) {
      continue;
    } else if (t_reply->elements % 2 != 0) {
      return HandleLogicError("MGetObjects::HGETALL: elements % 2 != 0", t_reply, false);
    }
    objects->push_back(GenObjectFromReply(t_reply));
    freeReplyObject(t_reply);
  }

  return Status::OK();
}

Status ZgwStore::AddMultiBlockSet(const std::string& bucket_name, const std::string& object_name,
    const std::string& upload_id, const std::string& block_index) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. SADD
 */
  std::string redis_key = kZgwMultiBlockSetPrefix + bucket_name + "_" +
    object_name + "_" + upload_id;
  redisReply* reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SADD %s %s", redis_key.c_str(), block_index.c_str()));
  if (reply == NULL) {
    return HandleIOError("AddMultiBlockSet::SADD");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("AddMultiBlockSet::SADD ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  if (reply->integer == 0) {
    return HandleLogicError("upload_id Already Exist", reply, false);
  }
  freeReplyObject(reply);
  return Status::OK();
}

Status ZgwStore::GetMultiBlockSet(const std::string& bucket_name, const std::string& object_name,
    const std::string& upload_id, std::vector<std::string>* block_indexs) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
  block_indexs->clear();
/*
 *  1. Get Block indexs
 */
  std::string redis_key = kZgwMultiBlockSetPrefix + bucket_name + "_" +
    object_name + "_" + upload_id;
  redisReply *reply;
  reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "SMEMBERS %s", redis_key.c_str()));
  if (reply == NULL) {
    return HandleIOError("GetMultiBlockSet::SEMBMBERS");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    return HandleLogicError("GetMultiBlockSet::SMEMBERS ret: " + std::string(reply->str), reply, false);
  }
  assert(reply->type == REDIS_REPLY_ARRAY);
  if (reply->elements == 0) {
    freeReplyObject(reply);
    return Status::OK();
  }
/*
 *  2. Iterate through indexs to push_back
 */
  for (unsigned int i = 0; i < reply->elements; i++) {
    block_indexs->push_back(reply->element[i]->str);
  }

  freeReplyObject(reply);
  return Status::OK();
}

Status ZgwStore::DeleteMultiBlockSet(const std::string& bucket_name, const std::string& object_name,
    const std::string& upload_id) {
  if (!MaybeHandleRedisError()) {
    return Status::IOError("Reconnect");
  }
  if (!CheckRedis()) {
    return Status::IOError("CheckRedis Failed");
  }
/*
 *  1. DEL
 */
  std::string redis_key = kZgwMultiBlockSetPrefix + bucket_name + "_" +
    object_name + "_" + upload_id;
  redisReply* reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "DEL %s", redis_key.c_str()));
  if (reply == NULL) {
    return HandleIOError("DeleteMultiBlockSet::DEL");
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);
  return Status::OK();
}

bool ZgwStore::MaybeHandleRedisError() {
  if (!redis_error_) {
    return true;
  }

  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  redis_cli_ = redisConnectWithTimeout(redis_ip_.c_str(), redis_port_, timeout);
  if (redis_cli_ == NULL || redis_cli_->err) {
    if (redis_cli_) {
      redisFree(redis_cli_);
    }
    return false;
  }

  if (!redis_passwd_.empty()) {
    redisReply* reply = static_cast<redisReply*>(redisCommand(redis_cli_,
                "AUTH %s", redis_passwd_.c_str()));
    if (reply == NULL) {
      return false;
    }
    if (std::string(reply->str) != "OK") {
      freeReplyObject(reply);
      return false;
    }
    freeReplyObject(reply);
  }

  redis_error_ = false;
  return true;
}

Status ZgwStore::HandleIOError(const std::string& func_name) {
  redisFree(redis_cli_);
  redis_error_ = true;
  return Status::IOError(func_name);
}

Status ZgwStore::HandleLogicError(const std::string& str_err, redisReply* reply,
    const bool should_unlock) {
  freeReplyObject(reply);
  Status s;
  if (should_unlock) {
    s = UnLock();
    return Status::Corruption(str_err + ", UnLock ret: " + s.ToString());
  }
  return Status::Corruption(str_err);
}

bool ZgwStore::CheckRedis() {
  redisReply* reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "PING"));
  if (reply != NULL &&
      reply->type == REDIS_REPLY_STATUS &&
      std::string(reply->str) == "PONG") {
    return true;
  }
  redis_error_ = true;
  return MaybeHandleRedisError();
}

User ZgwStore::GenUserFromReply(redisReply* reply) {
  User user;
  for (unsigned int i = 0; i < reply->elements; i++) {
    if (std::string(reply->element[i]->str) == "uid") {
      user.user_id = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "name") {
      user.display_name = reply->element[++i]->str;
      continue;
    } else {
      user.key_pairs.insert(std::pair<std::string, std::string>(reply->element[i]->str,
            reply->element[i+1]->str));
      i++;
      continue;
    }
  }
  return user;
}

Bucket ZgwStore::GenBucketFromReply(redisReply* reply) {
  Bucket bucket;
  char* end;
  for (unsigned int i = 0; i < reply->elements; i++) {
    if (std::string(reply->element[i]->str) == "name") {
      bucket.bucket_name = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "ctime") {
      bucket.create_time = std::strtoll(reply->element[++i]->str,
          &end, 10);
      continue;
    } else if (std::string(reply->element[i]->str) == "owner") {
      bucket.owner = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "acl") {
      bucket.acl = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "loc") {
      bucket.location = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "vol") {
      bucket.volumn = std::strtoull(reply->element[++i]->str,
          &end, 10);
      continue;
    } else if (std::string(reply->element[i]->str) == "uvol") {
      bucket.uploading_volumn = std::strtoull(reply->element[++i]->str,
          &end, 10);
    }
  }
  return bucket;
}

Object ZgwStore::GenObjectFromReply(redisReply* reply) {
  Object object;
  char* end;
  for (unsigned int i = 0; i < reply->elements; i++) {
    if (std::string(reply->element[i]->str) == "bname") {
      object.bucket_name = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "oname") {
      object.object_name = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "etag") {
      object.etag = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "size") {
      object.size = std::strtoll(reply->element[++i]->str,
          &end, 10);
      continue;
    } else if (std::string(reply->element[i]->str) == "owner") {
      object.owner = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "lm") {
      object.last_modified = std::strtoull(reply->element[++i]->str,
          &end, 10);
      continue;
    } else if (std::string(reply->element[i]->str) == "class") {
      object.storage_class = std::atoi(reply->element[++i]->str);
      continue;
    } else if (std::string(reply->element[i]->str) == "acl") {
      object.acl = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "id") {
      object.upload_id = reply->element[++i]->str;
      continue;
    } else if (std::string(reply->element[i]->str) == "block") {
      object.data_block = reply->element[++i]->str;
    }
  }
  return object;
}

Status ZgwStore::GetDeletedItem(std::string* item) {
  Status s;
  s = Lock();
  if (!s.ok()) {
    return s;
  }

  redisReply* reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "LPOP %s", kZgwDeletedList.c_str()));
  if (reply == NULL) {
    freeReplyObject(reply);
    return HandleIOError("GetDeteledItem::LPOP");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    freeReplyObject(reply);
    return HandleLogicError("PutDeletedItem::LPOP ret: " +
                            std::string(reply->str), reply, true);
  }
  bool empty = !(reply->type == REDIS_REPLY_STRING);
  if (!empty) {
    item->assign(reply->str);
  }

  freeReplyObject(reply);

  s = UnLock();
  if (!s.ok()) {
    return s;
  }

  return empty ? Status::NotFound("") : Status::OK();
}

Status ZgwStore::PutDeletedItem(const std::string& item, uint64_t deleted_time) {
  Status s = Lock();
  if (!s.ok()) {
    return s;
  }

  redisReply* reply = static_cast<redisReply*>(redisCommand(redis_cli_,
              "LPUSH %s %s", kZgwDeletedList.c_str(),
              std::string(item + "/" + std::to_string(deleted_time)).c_str()));
  if (reply == NULL) {
    return HandleIOError("PutDeletedItem::LPUSH");
  }
  if (reply->type == REDIS_REPLY_ERROR) {
    freeReplyObject(reply);
    return HandleLogicError("PutDeletedItem::LPUSH ret: " +
                            std::string(reply->str), reply, true);
  }
  assert(reply->type == REDIS_REPLY_INTEGER);
  freeReplyObject(reply);

  return UnLock();
}

}  // namespace zgwstore
