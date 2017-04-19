#include "src/libzgw/zgw_namelist.h"

#include <iostream>

#include "slash/include/slash_coding.h"

using slash::Slice;

namespace libzgw {

static const std::string kBucketsListPre = "__Buckets_list_";
static const std::string kObjectsListPre = "__Objects_list_";

std::string NameList::MetaValue() const {
  std::lock_guard<std::mutex> lock(list_lock);
  std::string value;
  slash::PutFixed32(&value, name_list.size());
  for (const auto& name : name_list) {
    slash::PutLengthPrefixedString(&value, name);
  }
  return value;
}

Status NameList::ParseMetaValue(std::string* value) {
  std::lock_guard<std::mutex> lock(list_lock);
  uint32_t count;
  slash::GetFixed32(value, &count);
  Slice svalue(*value);
  Slice name;
  for (uint32_t i = 0; i < count; i++) {
    bool res = slash::GetLengthPrefixedSlice(&svalue, &name);
    if (!res) {
      return Status::Corruption("Parse name failed");
    }
    name_list.insert(name.ToString());
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
  nl->Ref();
  *names = nl;

  return Status::OK();
}

Status ListMap::Unref(ZgwStore *store, const std::string &key) {
  Status s = Status::OK();
  std::lock_guard<std::mutex> lock(ref_lock_);
  if (map_list_.find(key) == map_list_.end()) {
    // Ignore
    return s;
  }
  NameList *nl = map_list_[key];
  bool should_flush = nl->Unref();

  if (should_flush) {
    if (nl->dirty()) {
      s = store->SaveNameList(nl);
      if (!s.ok()) {
        return s;
      }
      nl->SetDirty(false);
    }
    delete nl;
    map_list_.erase(key);
  }
  return s;
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
  Status s = store->GetNameList(new_list);
  if (!s.ok()) {
    delete new_list;
    return s;
  }

  map_list_.insert(std::make_pair(key, new_list));

  return Status::OK();
}

void NameList::Insert(const std::string &value) {
  std::lock_guard<std::mutex> lock(list_lock);
  name_list.insert(value);
  dirty_ = true;
}

void NameList::Delete(const std::string &value) {
  std::lock_guard<std::mutex> lock(list_lock);
  name_list.erase(value);
  dirty_ = true;
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
