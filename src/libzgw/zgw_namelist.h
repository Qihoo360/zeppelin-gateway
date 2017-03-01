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
      : refs_(0),
        key_(key),
        table_name_(table_name) {
  }
  void Ref();
  Status Unref(std::string &access_key, ZgwStore *store);

  std::string table_name() const {
    return table_name_;
  }

  const std::set<std::string> &name_list() const{
    return name_list_;
  }

  Status Insert(std::string &access_key,
                std::string &value,
                ZgwStore *store);

  Status Delete(std::string &access_key,
                std::string &value,
                ZgwStore *store);

  std::string MetaKey() const {
    return key_;
  }

  std::string MetaValue() const;

  Status ParseMetaValue(std::string *meta_value);

 private:
  std::mutex ref_mutex_;
  int refs_;
  std::string key_;
  std::string table_name_;
  std::set<std::string> name_list_;
};

struct ListMap {
  Status Insert(std::string &access_key,
                std::string &key,
                std::string &value,
                ZgwStore *store);
  Status Delete(std::string &access_key,
                std::string &key,
                std::string &value,
                ZgwStore *store);
  Status InitDataFromZp(std::string &access_key,
                        std::string &key,
                        ZgwStore *store);
  Status InitIfNeeded(std::string &access_key,
                      std::string &key,
                      ZgwStore *store);
  Status ListNames(std::string &access_key,
                   std::string &key,
                   NameList **names,
                   ZgwStore *store);
  std::mutex map_lock;
  std::map<std::string, NameList *> map_list;
  int key_type;
  enum KEY_TYPE {
    kBuckets,
    kObjects
  };
};

}  // namespace libzgw

#endif  // ZGW_NAMELIST_H
