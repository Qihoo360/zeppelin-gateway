#include "zgw_store.h"

#include <unistd.h>

#include "slash/include/slash_string.h"
#include "zgw_user.h"

namespace libzgw {

Status ZgwStore::BuildMap() {
  Status s;
  std::string meta_value;
  for (auto &name : user_list_.users_name) {
    ZgwUser *user = new ZgwUser(name);
    s = zp_->Get(kZgwMetaTableName, user->MetaKey(), &meta_value);
    if (!s.ok()) {
      return s;
    }
    s = user->ParseMetaValue(&meta_value);
    if (!s.ok()) {
      return s;
    }

    for (auto &key_pair : user->access_keys()) {
      //                    access key
      access_key_user_map_[key_pair.first] = user;
    }
  }

  return Status::OK();
}

Status ZgwStore::LoadAllUsers() {
  Status s;
  // Load all users
  std::string meta_value;
  int retry = 3;
  do {
    s = zp_->Get(kZgwMetaTableName, user_list_.MetaKey(), &meta_value);
    if (s.ok()) {
      break;
    }
  } while (retry--);

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
  // Return if user exists
  if (user_list_.users_name.find(user_name) !=
      user_list_.users_name.end()) {
    return Status::Corruption("User already created");
  }

  // Create user
  ZgwUser *user = new ZgwUser(user_name);
  Status s = user->GenKeyPair(access_key, secret_key);
  if (!s.ok()) {
    return s;
  }

  // Dump user to zeppelin
  s = zp_->Set(kZgwMetaTableName, user->MetaKey(), user->MetaValue());
  if (!s.ok()) {
    return s;
  }
  
  // Dump user to zeppelin
  s = zp_->Set(kZgwMetaTableName, user->MetaKey(), user->MetaValue());
  if (!s.ok()) {
    return s;
  }

  // Insert into memory
  access_key_user_map_[*access_key] = user;
  user_list_.users_name.insert(user_name);

  // Dump list to zeppelin
  s = zp_->Set(kZgwMetaTableName, user_list_.MetaKey(), user_list_.MetaValue());
  if (!s.ok()) {
    user_list_.users_name.erase(user_name);
    access_key_user_map_.erase(*access_key);
    return s;
  }

  return s;
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
