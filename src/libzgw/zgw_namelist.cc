#include "zgw_namelist.h"

#include <iostream>

#include "slash/include/slash_coding.h"

namespace libzgw {

static const std::string kBucketsListPre = "__Buckets_list_";
static const std::string kObjectsListPre = "__Objects_list_";

std::string NameList::MetaValue() const {
  std::lock_guard<std::mutex> lock(list_lock);
  std::string value;
  slash::PutFixed32(&value, name_list.size());
  for (auto &name : name_list) {
    slash::PutLengthPrefixedString(&value, name);
  }
  return value;
}

Status NameList::ParseMetaValue(std::string *value) {
  std::lock_guard<std::mutex> lock(list_lock);
  uint32_t count;
  slash::GetFixed32(value, &count);
  std::string name;
  for (uint32_t i = 0; i < count; i++) {
    bool res = slash::GetLengthPrefixedString(value, &name);
    if (!res) {
      return Status::Corruption("Parse name failed");
    }
    name_list.insert(name);
  }

  return Status::OK();
}

Status ListMap::Ref(ZgwStore *store, const std::string key, NameList **names) {
  std::lock_guard<std::mutex> lock(ref_lock_);
  Status s;
  NameList *l_names;
  if (map_list_.find(key) == map_list_.end()) {
    s = InitNameList(key, store, &l_names);
    if (!s.ok()) {
      *names = NULL;
      return s;
    }
  }
  NameList *nl = map_list_[key];
  nl->ref += 1;
  std::cout << key + " after Ref: " << nl->ref << std::endl;
  *names = nl;

  return Status::OK();
}

Status ListMap::Unref(ZgwStore *store, const std::string &key) {
  ref_lock_.lock();
  if (map_list_.find(key) == map_list_.end()) {
    // Ignore
    ref_lock_.unlock();
    return Status::OK();
  }
  NameList *nl = map_list_[key];
  nl->ref -= 1;
  std::cout << key + " after Unref: " << nl->ref << std::endl;

  if (nl->ref <= 0) {
    nl->ref = 0; // Avoid multi unref
    Status s = nl->Save(store);
    if (!s.ok()) {
      ref_lock_.unlock();
      return s;
    }
    delete nl;
    map_list_.erase(key);
  }
  ref_lock_.unlock();
  return Status::OK();
}

Status ListMap::InitNameList(const std::string &key, ZgwStore *store,
                             NameList **names) {
  // Read from zp to new list
  NameList *new_list;
  if (key_type_ == kBuckets) {
    // talbe name: kUserTableName
    // meta key: __Buckets_list_<user_name>
    new_list = new NameList(kBucketsListPre + key);
  } else if (key_type_ == kObjects) {
    // table name: <bucket_name>
    // meta key: __Objects_list_<bucket_name>
    new_list = new NameList(kObjectsListPre + key);
  } else {
    return Status::NotSupported("Unknow key type");
  }
  Status s = new_list->Load(store);
  if (!s.ok()) {
    delete new_list;
    return s;
  }

  new_list->ref = 0;
  map_list_[key] = new_list;

  return Status::OK();
}

Status NameList::Load(ZgwStore *store) {
  std::string meta_value;
  Status s = store->GetNameList(meta_key, &meta_value);
  if (s.ok()) {
    return ParseMetaValue(&meta_value);
  } else if (s.IsNotFound()) {
    return Status::OK();
  }
  return s;
}

Status NameList::Save(ZgwStore *store) {
  if (dirty) {
    Status s = store->SaveNameList(meta_key, MetaValue());
    if (!s.ok()) {
      return s;
    }
    dirty = false;
  }
  return Status::OK();
}

void NameList::Insert(const std::string &value) {
  std::lock_guard<std::mutex> lock(list_lock);
  name_list.insert(value);
  dirty = true;
}

void NameList::Delete(const std::string &value) {
  std::lock_guard<std::mutex> lock(list_lock);
  name_list.erase(value);
  dirty = true;
}

bool NameList::IsExist(const std::string &value) {
  std::lock_guard<std::mutex> lock(list_lock);
  return (name_list.find(value) != name_list.end());
}

bool NameList::IsEmpty() {
  std::lock_guard<std::mutex> lock(list_lock);
  return name_list.empty();
}

}  // namespace libzgw
