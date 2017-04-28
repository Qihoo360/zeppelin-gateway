#include "src/libzgw/zgw_store.h"

#include <unistd.h>

#include "slash/include/slash_string.h"

namespace libzgw {

ZgwStore::ZgwStore() {
}

ZgwStore::~ZgwStore() {
  delete zp_;
  for (auto &item : access_key_user_map_) {
    delete item.second;
  }
}

Status ZgwStore::Open(const std::vector<std::string>& ip_ports, ZgwStore** ptr) {
  ZgwStore* zgw_store = new ZgwStore();
  Status s = zgw_store->Init(ip_ports);
  if (!s.ok()) {
    delete zgw_store;
  }
  *ptr = zgw_store;
  return s;
}

Status ZgwStore::Init(const std::vector<std::string>& ip_ports) {
  if (ip_ports.empty()) {
    return Status::InvalidArgument("no meta ip provided");
  }

  std::string t_ip;
  int t_port = 0;
  libzp::Options zp_option;
  for (auto& node : ip_ports) {
    if (!slash::ParseIpPortString(node, t_ip, t_port)) {
      return Status::InvalidArgument("invalid ip port string");
    }
    zp_option.meta_addr.push_back(libzp::Node(t_ip, t_port));
  }
  zp_ = new libzp::Cluster(zp_option);
  assert(zp_);

  // Find meta and data tables
  std::vector<std::string> tables;
  bool meta_table_found = false;
  bool data_table_found = false;
  Status s = zp_->ListTable(&tables);
  if (s.IsIOError()) {
    return s;
  }
  for (auto &table : tables) {
    if (table == kZgwMetaTableName) {
      meta_table_found = true;
    }
    if (table == kZgwDataTableName) {
      data_table_found = true;
    }
  }
  // Create Bucket if not exist
  Status s1 = Status::OK();
  s = Status::OK();
  if (!meta_table_found) {
    s = zp_->CreateTable(kZgwMetaTableName, kZgwTablePartitionNum);
  }
  if (!data_table_found) {
    s1 = zp_->CreateTable(kZgwDataTableName, kZgwTablePartitionNum);
  }
  if (s.IsIOError()) {
    return s;
  } else if (s1.IsIOError()) {
    return s1;
  } else {
    // Alread create
  }
  if (!meta_table_found || !data_table_found) {
    // Waiting for table created
    sleep(10);
  }

  // Load all users
  s = LoadAllUsers();
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

Status ZgwStore::SaveNameList(const NameList* nlist) {
  return zp_->Set(kZgwMetaTableName, nlist->MetaKey(), nlist->MetaValue());
}

Status ZgwStore::GetNameList(NameList* nlist) {
  Status s;
  std::string meta_value;
  s = zp_->Get(kZgwMetaTableName, nlist->MetaKey(), &meta_value);
  if (s.ok()) {
    return nlist->ParseMetaValue(&meta_value);
  } else if (s.IsNotFound()) {
    return Status::OK();
  }
  return s;
}

}  // namespace libzgw
