#include "zgw_namelist.h"
#include "include/slash_coding.h"

namespace libzgw {

static const std::string kBucketsListPre = "__Buckets_list_";
static const std::string kObjectsListPre = "__Objects_list_";

void NameList::Ref() {
  ++refs_;
}

Status NameList::Unref(std::string &access_key, ZgwStore *store) {
  --refs_;
  assert(refs_ >= 0);
  if (refs_ == 0) {
    // Dump to zp
    Status s = store->SaveNameList(access_key, this);
    if (!s.ok()) {
      return s;
    }
  }
  return Status::OK();
}

std::string NameList::MetaValue() const {
  std::string value;
  slash::PutFixed32(&value, name_list_.size());
  for (auto &name : name_list_) {
    slash::PutLengthPrefixedString(&value, name);
  }
  return value;
}

Status NameList::ParseMetaValue(std::string *value) {
  uint32_t count;
  slash::GetFixed32(value, &count);
  std::string name;
  for (int i = 0; i < count; i++) {
    bool res = slash::GetLengthPrefixedString(value, &name);
    if (!res) {
      return Status::Corruption("Parse name failed");
    }
    name_list_.insert(name);
  }

  return Status::OK();
}

Status NameList::Insert(std::string &access_key,
                        std::string &value,
                        ZgwStore *store) {
  std::lock_guard<std::mutex> lock(ref_mutex_);
  Ref();
  name_list_.insert(value);
  Status s = Unref(access_key, store);
  if (!s.ok()) {
    name_list_.erase(value);
    return s;
  }
  return Status::OK();
}

Status NameList::Delete(std::string &access_key,
                        std::string &value,
                        ZgwStore *store) {
  std::lock_guard<std::mutex> lock(ref_mutex_);
  Ref();
  name_list_.erase(value);
  Status s = Unref(access_key, store);
  if (!s.ok()) {
    name_list_.insert(value);
    return s;
  }
  return Status::OK();
}

Status ListMap::InitDataFromZp(std::string &access_key,
                               std::string &key,
                               ZgwStore *store) {
  // Read from zp to new list
  NameList *new_list;
  if (key_type == kBuckets) {
    // talbe name: kUserTableName
    // meta key: __Buckets_list_<user_name>
    new_list = new NameList(kUserTableName, kBucketsListPre + key);
  } else if (key_type == kObjects) {
    // table name: <bucket_name>
    // meta key: __Objects_list_<bucket_name>
    new_list = new NameList(key, kObjectsListPre + key);
  } else {
    return Status::NotSupported("Unknow key type");
  }

  // Read from zp
  Status s = store->GetNameList(access_key, new_list);
  if (!s.ok() && !s.IsNotFound()) {
    return s;
  }
  map_list[key] = new_list;

  return Status::OK();
}

Status ListMap::InitIfNeeded(std::string &access_key,
                             std::string &key,
                             ZgwStore *store) {
  Status s;
  std::lock_guard<std::mutex> lock(map_lock);
  const auto &nlist = map_list.find(key);
  if (nlist == map_list.end()) {
    s = InitDataFromZp(access_key, key, store);
    if (!s.ok()) {
      return s;
    }
  }

  return Status::OK();
}

Status ListMap::Insert(std::string &access_key,
                       std::string &key,
                       std::string &value,
                       ZgwStore *store) {
  Status s = InitIfNeeded(access_key, key, store);
  if (!s.ok()) {
    return s;
  }
  auto &nlist = *map_list.find(key);
  return nlist.second->Insert(access_key, value, store);
}

Status ListMap::Delete(std::string &access_key,
                       std::string &key,
                       std::string &value,
                       ZgwStore *store) {
  Status s = InitIfNeeded(access_key, key, store);
  if (!s.ok()) {
    return s;
  }
  auto &nlist = *map_list.find(key);
  return nlist.second->Delete(access_key, value, store);
}

Status ListMap::ListNames(std::string &access_key,
                          std::string &key,
                          NameList **names,
                          ZgwStore *store) {
  Status s = InitIfNeeded(access_key, key, store);
  if (!s.ok()) {
    return s;
  }
  auto &nlist = *map_list.find(key);
  *names = nlist.second;
  return Status::OK();
}

}  // namespace libzgw
