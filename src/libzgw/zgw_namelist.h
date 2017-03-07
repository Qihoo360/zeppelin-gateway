#ifndef ZGW_NAMELIST_H
#define ZGW_NAMELIST_H

#include <string>
#include <set>
#include <map>
#include <mutex>

#include <include/slash_status.h>
#include "zgw_store.h"

using slash::Status;

namespace libzgw {

class ZgwStore;

class NameList {
public:
  explicit NameList(const std::string &table_name, std::string key)
      : dirty(false),
        meta_key(key),
        table_name(table_name) {
  }

  Status Load(ZgwStore *store);
  Status Save(ZgwStore *store);

  void Insert(std::string &value);

  void Delete(std::string &value);

  bool IsExist(std::string &value) {
    return (name_list.find(value) != name_list.end());
  }

  std::string MetaValue() const;

  Status ParseMetaValue(std::string *meta_value);

  bool dirty;
  int ref;
  std::mutex list_lock;
  std::string meta_key;
  std::string table_name;
  std::set<std::string> name_list;
};

class ListMap {
 public:
  enum KEY_TYPE {
    kBuckets,
    kObjects
  };
  ListMap(int key_type)
      : key_type_(key_type) {
  }

  Status Ref(ZgwStore *store, const std::string key, NameList **names);

  Status Unref(ZgwStore *store, const std::string &key);

  Status InitNameList(const std::string &key, ZgwStore *store, NameList **names);

 private:
  std::mutex ref_lock_;
  //           key                ref    namelist
  std::map<std::string, NameList *> map_list_;
  int key_type_;
};

}  // namespace libzgw

#endif  // ZGW_NAMELIST_H
