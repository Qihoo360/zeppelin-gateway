#include <unistd.h>
#include "zgw_store.h"

#include "include/slash_string.h"

namespace libzgw {

Status ZgwStore::Open(const std::vector<std::string>& ip_ports, ZgwStore** ptr) {
  *ptr = new ZgwStore();
  assert(*ptr);
  Status s = (*ptr)->Init(ip_ports);
  if (!s.ok()) {
    delete *ptr;
  }
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

  // Create Bucket
  Status s = zp_->CreateTable(kZgwMetaTableName, kZgwTablePartitionNum);
  Status s1 = zp_->CreateTable(kZgwDataTableName, kZgwTablePartitionNum);
  if (s.IsIOError()) {
    return s;
  } else if (s1.IsIOError()) {
    return s1;
  } else {
    // Alread create
  }
  sleep(10);

  // Load all users
  s = LoadAllUsers();
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

ZgwStore::ZgwStore() {
}

ZgwStore::~ZgwStore() {
  delete zp_;
  for (auto &item : access_key_user_map_) {
    delete item.second;
  }
}

}  // namespace libzgw
