#include <sys/time.h>
#include "zgw_user.h"

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
  // access keys
  slash::PutFixed32(&result, access_keys_.size());
  for (auto &access_key : access_keys_) {
    slash::PutLengthPrefixedString(&result, access_key);
  }
  // secret keys
  slash::PutFixed32(&result, secret_keys_.size());
  for (auto &secret_key : secret_keys_) {
    slash::PutLengthPrefixedString(&result, secret_key);
  }
  // buckets name
  slash::PutFixed32(&result, buckets_name_.size());
  for (auto &bucket_name : buckets_name_) {
    slash::PutLengthPrefixedString(&result, bucket_name);
  }
  return result;
}

Status ZgwUser::ParseMetaValue(std::string* value) {
  // user info
  std::string user_info;
  slash::GetLengthPrefixedString(value, &user_info);
  info_.ParseMetaValue(&user_info);
  // access keys
  std::string tmp_str;
  uint32_t access_key_count_;
  slash::GetFixed32(value, &access_key_count_);
  for (size_t i = 0; i < access_key_count_; ++i) {
    slash::GetLengthPrefixedString(value, &tmp_str);
    access_keys_.insert(tmp_str);
  }
  if (access_keys_.size() != access_key_count_) {
    return Status::Corruption("Parse access keys failed");
  }

  // secret keys
  uint32_t secret_key_count_;
  slash::GetFixed32(value, &secret_key_count_);
  for (size_t i = 0; i < secret_key_count_; ++i) {
    slash::GetLengthPrefixedString(value, &tmp_str);
    secret_keys_.insert(tmp_str);
  }
  if (secret_keys_.size() != secret_key_count_) {
    return Status::Corruption("Parse access keys failed");
  }

  // buckets name
  uint32_t buckets_name_count_;
  slash::GetFixed32(value, &buckets_name_count_);
  for (size_t i = 0; i < buckets_name_count_; ++i) {
    slash::GetLengthPrefixedString(value, &tmp_str);
    buckets_name_.insert(tmp_str);
  }
  if (buckets_name_.size() != buckets_name_count_) {
    return Status::Corruption("Parse access keys failed");
  }

  return Status::OK();
}

Status ZgwUser::GenAccessKey(std::string *access_key) {
  std::string key;
  int retry = 3;
  while (retry--) {
    key = GenRandomKey(20);
    if (access_keys_.find(key) == access_keys_.end()) {
      access_keys_.insert(key);
      access_key->assign(key);
      return Status::OK();
    }
  }
  return Status::Corruption("Generate access key failed");
}

Status ZgwUser::GenSecretKey(std::string *secret_key) {
  std::string key;
  int retry = 3;
  while (retry--) {
    key = GenRandomKey(40);
    if (secret_keys_.find(key) == secret_keys_.end()) {
      secret_keys_.insert(key);
      secret_key->assign(key);
      return Status::OK();
    }
  }
  return Status::Corruption("Generate secret key failed");
}

std::string ZgwUser::GenRandomKey(int width) {
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
