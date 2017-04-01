#include "zgw_user.h"

#include <sys/time.h>

namespace libzgw {

std::string ZgwUserList::MetaValue() const {
  std::string result;
  slash::PutFixed32(&result, users_name.size());
  for (auto &name : users_name) {
    slash::PutLengthPrefixedString(&result, name);
  }
  return result;
}

Status ZgwUserList::ParseMetaValue(std::string* value) {
  uint32_t count;
  std::string name;
  slash::GetFixed32(value, &count);
  for (size_t i = 0; i < count; ++i) {
    bool res = slash::GetLengthPrefixedString(value, &name);
    if (!res) {
      return Status::Corruption("Parse user list failed");
    }
    users_name.insert(name);
  }
  if (count != users_name.size()) {
    return Status::Corruption("Parse users name failed");
  }
  return Status::OK();
}

std::string ZgwUserInfo::MetaValue() const {
  std::string result;
  // User Internal meta
  slash::PutLengthPrefixedString(&result, user_id);
  slash::PutLengthPrefixedString(&result, disply_name);
  return result;
}

Status ZgwUserInfo::ParseMetaValue(std::string* value) {
  // User Interal meta
  bool res = slash::GetLengthPrefixedString(value, &user_id);
  if (!res) {
    return Status::Corruption("Parse user_id failed");
  }
  res = slash::GetLengthPrefixedString(value, &disply_name);
  if (!res) {
    return Status::Corruption("Parse disply_name failed");
  }
  return Status::OK();
}

std::string ZgwUser::MetaValue() const {
  std::string result;
  // user info
  slash::PutLengthPrefixedString(&result, info_.MetaValue());

  slash::PutFixed32(&result, key_pairs_.size());
  for (auto &key_pair : key_pairs_) {
    // access key
    slash::PutLengthPrefixedString(&result, key_pair.first);
    // secret key
    slash::PutLengthPrefixedString(&result, key_pair.second);
  }

  return result;
}

Status ZgwUser::ParseMetaValue(std::string* value) {
  // user info
  std::string user_info;
  slash::GetLengthPrefixedString(value, &user_info);
  info_.ParseMetaValue(&user_info);

  std::string tmp_str1;
  std::string tmp_str2;
  bool res;
  uint32_t key_pairs_count;
  slash::GetFixed32(value, &key_pairs_count);
  for (size_t i = 0; i < key_pairs_count; ++i) {
    // access key
    if (!slash::GetLengthPrefixedString(value, &tmp_str1)) {
      return Status::Corruption("Parse access key failed");
    }
    // secret key
    if (!slash::GetLengthPrefixedString(value, &tmp_str2)) {
      return Status::Corruption("Parse secret key failed");
    }
    key_pairs_.insert(std::make_pair(tmp_str1, tmp_str2));
  }

  return Status::OK();
}

Status ZgwUser::GenKeyPair(std::string *access_key, std::string *secret_key) {
  std::string key1, key2;
  int retry = 10;
  while (retry--) {
    key1 = GenRandomStr(20);
    key2 = GenRandomStr(40);
    if (key_pairs_.find(key1) == key_pairs_.end()) {
      access_key->assign(key1);
      secret_key->assign(key2);
      key_pairs_[key1] = key2;
      return Status::OK();
    }
  }
  return Status::Corruption("Generate access key pair failed");
}

std::string ZgwUser::GenRandomStr(int width) {
  timeval now;
  gettimeofday(&now, NULL);
  srand(now.tv_usec);
  std::string key;
  char x;
  for (int i = 0; i < width; ++i) {
    do {
      x = static_cast<char>(rand());
    } while (x < '0' ||
             (x > '9' && x < 'A') ||
             (x > 'Z' && x < 'a') ||
             x > 'z');
    key.push_back(x);
  }
  return key;
}

}  // namespace libzgw
