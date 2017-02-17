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

Status ZgwStore::AddBucket(const ZgwBucket& bucket,
    int partition_num) {
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Create Bucket
  s = zp_->CreateTable(bucket.name(), partition_num);
  if (!s.ok()) {
    return s;
  }

  // Add Bucket Meta
  return zp_->Set(bucket.name(), bucket.MetaKey(), bucket.MetaValue());
}

Status ZgwStore::ListBucket(std::vector<ZgwBucket*>* buckets) {
  assert(buckets);
  Status s = zp_->Connect();
  if (!s.ok()) {
    return s;
  }

  // Get all Bucket name 
  std::vector<std::string> bucket_names;
  s = zp_->ListTable(&bucket_names);
  if (!s.ok()) {
    return s;
  }
  
  // Get Bucket Meta
  if (!bucket_names.empty()) {
    for (std::string& name : bucket_names) {
      std::string value;
      ZgwBucket obucket(name);
      zp_->Get(name, obucket.MetaKey(), &value);
      if (!obucket.ParseMetaValue(value).ok()) {
        continue; // Skip table with not meta info
      }
      buckets->push_back(&obucket);
    }
  }

  return Status::OK();
}


}
