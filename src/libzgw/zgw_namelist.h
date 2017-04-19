#ifndef ZGW_NAMELIST_H
#define ZGW_NAMELIST_H

#include <iostream>
#include <string>
#include <set>
#include <map>
#include <mutex>

#include "slash/include/slash_status.h"
#include "src/libzgw/zgw_store.h"

using slash::Status;

namespace libzgw {

class ZgwStore;

class NameList {
 public:
  explicit NameList(std::string key)
      : dirty_(false),
        ref_(0),
        meta_key_(std::move(key)) {
  }

  bool dirty() const {
    return dirty_;
  }

  void SetDirty(bool value) {
    dirty_ = value;
  }

  void Ref() {
    ++ref_;
    std::cerr << meta_key_ + " after Ref: " << ref_ << std::endl;
  }

  bool Unref() {
    --ref_;
    std::cerr << meta_key_ + " after Unref: " << ref_ << std::endl;
    if (ref_ <= 0) {
      ref_ = 0; // Avoid multi unref
      return true;
    }
    return false;
  }

  void Insert(const std::string& value);
  void Delete(const std::string& value);
  bool IsExist(const std::string& value);
  bool IsEmpty();

  std::string MetaKey() const {
    return meta_key_;
  }
  std::string MetaValue() const;
  Status ParseMetaValue(std::string* meta_value);

  mutable std::mutex list_lock;
  std::set<std::string> name_list;

 private:
  bool dirty_;
  int ref_;
  std::string meta_key_;
};

class ListMap {
 public:
  enum KEY_TYPE {
    kBuckets,
    kObjects
  };
  ListMap(KEY_TYPE key_type)
      : key_type_(key_type) {
  }

  Status Ref(ZgwStore* store, const std::string key, NameList** names);

  Status Unref(ZgwStore* store, const std::string& key);

  Status InitNameList(const std::string& key, ZgwStore* store, NameList** names);

 private:
  std::mutex ref_lock_;
  //           key                ref    namelist
  std::map<std::string, NameList*> map_list_;
  int key_type_;
};

}  // namespace libzgw

#endif  // ZGW_NAMELIST_H
