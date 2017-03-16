#include "zgw_server.h"

#include <glog/logging.h>

Status ZgwWorkerThread::Init(std::vector<std::string> &zp_meta_ip_ports) {
  // Open ZgwStore
  Status s =
    libzgw::ZgwStore::Open(zp_meta_ip_ports, &store_); 
  if (!s.ok()) {
    LOG(FATAL) << "Can not open ZgwStore: " << s.ToString();
    return s;
  }

  return Status::OK();
}

ZgwWorkerThread::~ZgwWorkerThread() {
  delete store_;
}

ZgwServer::ZgwServer(ZgwConfig *zgw_conf)
    : zgw_conf_(zgw_conf),
      ip_(zgw_conf->server_ip),
      port_(zgw_conf->server_port),
      should_exit_(false) {
  worker_num_ = zgw_conf->worker_num;
  for (int i = 0; i < worker_num_; i++) {
    zgw_worker_thread_[i] = new ZgwWorkerThread();
  }
  std::set<std::string> ips;
  ips.insert(ip_);
  zgw_dispatch_thread_ = new pink::DispatchThread<ZgwConn>(
      ips,
      port_,
      worker_num_,
      reinterpret_cast<pink::WorkerThread<ZgwConn> **>(zgw_worker_thread_),
      kDispatchCronInterval);
  buckets_list_ = new libzgw::ListMap(libzgw::ListMap::kBuckets);
  objects_list_ = new libzgw::ListMap(libzgw::ListMap::kObjects);
}

ZgwServer::~ZgwServer() {
  delete zgw_dispatch_thread_;
  for (int i = 0; i < worker_num_; i++) {
    delete zgw_worker_thread_[i];
  }

  LOG(INFO) << "ZgwServerThread " << pthread_self() << " exit!!!";
}

Status ZgwServer::Start() {
  Status s;
  LOG(INFO) << "Waiting for ZgwServerThread Init, About "<< worker_num_ * 10 << "s";
  for (int i = 0; i < worker_num_; i++) {
    s = zgw_worker_thread_[i]->Init(zgw_conf_->zp_meta_ip_ports);
    if (!s.ok()) {
      return s;
    }
  }
  
  int ret = zgw_dispatch_thread_->StartThread();
  if (ret != 0) {
    return Status::Corruption("Launch DispatchThread failed");
  }
  LOG(INFO) << "ZgwServerThread Init Success!";

  while (!should_exit_) {
    DoTimingTask();
    usleep(kZgwCronInterval * 1000);
  }
  return Status::OK();
}

void ZgwServer::DoTimingTask() {}
