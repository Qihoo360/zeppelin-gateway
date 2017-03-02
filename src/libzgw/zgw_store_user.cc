#include <unistd.h>

#include "zgw_store.h"
#include "zgw_user.h"

#include "include/slash_string.h"

namespace libzgw {

Status ZgwStore::BuildMap() {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  std::string meta_value;
  for (auto &name : user_list_.users_name) {
    ZgwUser *user = new ZgwUser(name);
    s = zp_->Get(kUserTableName, user->MetaKey(), &meta_value);
    if (!s.ok()) {
      return s;
    }
    s = user->ParseMetaValue(&meta_value);
    if (!s.ok()) {
      return s;
    }

    for (auto &ac_key : user->access_keys()) {
      access_key_user_map_[ac_key] = user;
    }
  }

  return Status::OK();
}

Status ZgwStore::LoadAllUsers() {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Create user infomation table if not exist
  s = zp_->CreateTable(kUserTableName, kUserTablePartionNum);
  if (!s.ok() &&  // TODO (gaodq) more reasonable implement
      !s.IsNotSupported()) {
    // Can not create user infomation table
    return s;
  }

  // Load all users
  std::string meta_value;
  int retry = 10;
  do {
    s = zp_->Get(kUserTableName, user_list_.MetaKey(), &meta_value);
    if (s.ok()) {
      break;
    }
    sleep(2);
  } while (retry-- && s.IsIOError());

  if (!s.ok() && !s.IsNotFound()) {
    return s;
  }

  if (!meta_value.empty() &&
      user_list_.ParseMetaValue(&meta_value).ok()) {
    return BuildMap();
  }
  // Empty user list
  return Status::OK();
}
 
Status ZgwStore::AddUser(const std::string &user_name,
                         std::string *access_key,
                         std::string *secret_key) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Return if user exists
  if (user_list_.users_name.find(user_name) !=
      user_list_.users_name.end()) {
    return Status::Corruption("User already created");
  }

  // Create user
  ZgwUser *user = new ZgwUser(user_name);
  s = user->GenAccessKey(access_key);
  if (!s.ok()) {
    return s;
  }
  s = user->GenSecretKey(secret_key);
  if (!s.ok()) {
    return s;
  }
  // Insert into memory
  user_list_.users_name.insert(user_name);
  access_key_user_map_[*access_key] = user;
  // Dump to zeppelin
  s = zp_->Set(kUserTableName, user_list_.MetaKey(), user_list_.MetaValue());
  if (!s.ok()) {
    return s;
  }
  return zp_->Set(kUserTableName, user->MetaKey(), user->MetaValue());
}

Status ZgwStore::GetUser(const std::string &access_key, ZgwUser **user) {
  assert(user);
  int times = 0;
retry:
  bool found = (access_key_user_map_.find(access_key) !=
                access_key_user_map_.end());

  if (!found) {
    if (times == 0) {
      ++times;
      LoadAllUsers();
      goto retry;
    } else {
      return Status::AuthFailed("Can not recognize this access key");
    }
  }

  *user = access_key_user_map_[access_key];
  return Status::OK();
}

// Use std::set avoid repeated user
Status ZgwStore::ListUsers(std::set<ZgwUser *> *user_list) {
  Status s = LoadAllUsers();
  if (!s.ok()) {
    return s;
  }
  for (auto &item : access_key_user_map_) {
    user_list->insert(item.second);
  }

  return Status::OK();
}

}  // namespace libzgw
