#include "zgw_store.h"

#include "include/slash_string.h"

namespace libzgw {
 
Status ZgwStore::Open(const std::string& name,
    const std::vector<std::string>& ips, ZgwStore** ptr) {
  *ptr = new ZgwStore(name);
  assert(*ptr);
  Status s = (*ptr)->Init(ips);
  if (!s.ok()) {
    delete *ptr;
  }
  return s;
}

Status ZgwStore::Init(const std::vector<std::string>& ips) {
  if (ips.empty()) {
    return Status::InvalidArgument("no meta ip provided");
  }

  std::string t_ip;
  int t_port = 0;
  libzp::Options zp_option;
  for (auto& node : ips) {
    if (!slash::ParseIpPortString(node, t_ip, t_port)) {
      return Status::InvalidArgument("invalid ip port string");
    }
    zp_option.meta_addr.push_back(libzp::Node(t_ip, t_port));
  }
  zp_ = new libzp::Cluster(zp_option);
  assert(zp_);
  return Status::OK();
}

ZgwStore::ZgwStore(const std::string& name)
  :name_(name) {
}

ZgwStore::~ZgwStore() {
  delete zp_;
}

}
