#ifndef ZGW_USER_H
#define ZGW_USER_H
#include <string>
#include <set>
#include <map>
#include "time.h"

#include "include/slash_status.h"
#include "include/slash_coding.h"
#include "include/slash_hash.h"

namespace libzgw {

static const std::string kUserMetaPrefix = "__U__";
static const std::string kUserListKey = "__ZGW_userlist";

using slash::Status;

struct ZgwUserList {
  std::set<std::string> users_name;

  // Serialization
  std::string MetaKey() const {
    return kUserListKey;
  }
  // Serialization
  std::string MetaValue() const;
  // Deserialization
  Status ParseMetaValue(std::string* value);
};

struct ZgwUserInfo {
  std::string disply_name; // unique
  std::string user_id; // unique

  // Serialization
  std::string MetaValue() const;
  // Deserialization
  Status ParseMetaValue(std::string* value);
};

class ZgwUser {
 public:
  explicit ZgwUser(std::string user_name) {
    info_.disply_name = user_name;
    info_.user_id = slash::sha256(user_name);
  }
  ~ZgwUser() {}

  const ZgwUserInfo &user_info() const {
    return info_;
  }

  Status GenKeyPair(std::string *access_key, std::string *secret_key);

  std::map<std::string, std::string> &access_keys() {
    return key_pairs_;
  }

  std::string secret_key(std::string access_key) {
    return key_pairs_[access_key];
  }

  // Serialization
  std::string MetaKey() const {
    return kUserMetaPrefix + info_.user_id;
  }
  std::string MetaValue() const;
  // Deserialization
  Status ParseMetaValue(std::string* value);

 private:
  ZgwUserInfo info_;
  //       access key,  secret key
  std::map<std::string, std::string> key_pairs_;

  std::string GenRandomStr(int width);
};

}  // namespace libzgw

#endif
