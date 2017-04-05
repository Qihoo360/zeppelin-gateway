#include "zgw_server.h"

#include <glog/logging.h>

#include "zgw_conn.h"

Status ZgwServer::InitWorderThread(pink::Thread* worker,
                                   std::vector<std::string> &zp_meta_ip_ports) {
  // Open ZgwStore
  libzgw::ZgwStore* store;
  Status s =
    libzgw::ZgwStore::Open(zp_meta_ip_ports, &store); 
  if (!s.ok()) {
    LOG(FATAL) << "Can not open ZgwStore: " << s.ToString();
    return s;
  }
  std::lock_guard<std::mutex> lock(worker_store_mutex_);
  worker_store_.insert(std::make_pair(worker, store));

  return Status::OK();
}

libzgw::ZgwStore* ZgwServer::GetWorkerStore(pink::Thread* worker) {
  std::lock_guard<std::mutex> lock(worker_store_mutex_);
  if (worker_store_.find(worker) != worker_store_.end()) {
    return worker_store_[worker];
  }
  return NULL;
}

ZgwServer::ZgwServer(ZgwConfig *zgw_conf)
    : zgw_conf_(zgw_conf),
      ip_(zgw_conf->server_ip),
      port_(zgw_conf->server_port),
      should_exit_(false) {
  worker_num_ = zgw_conf->worker_num;
  if (worker_num_ > kMaxWorkerThread) {
    LOG(WARNING) << "Exceed max worker thread num: " << kMaxWorkerThread;
    worker_num_ = kMaxWorkerThread;
  }
  conn_factory = new ZgwConnFactory();
  for (int i = 0; i < worker_num_; i++) {
    zgw_worker_thread_[i] = pink::NewWorkerThread(conn_factory);
  }
  std::set<std::string> ips;
  ips.insert(ip_);
  zgw_dispatch_thread_ = pink::NewDispatchThread(ips, port_, worker_num_, zgw_worker_thread_);
  buckets_list_ = new libzgw::ListMap(libzgw::ListMap::kBuckets);
  objects_list_ = new libzgw::ListMap(libzgw::ListMap::kObjects);
}

ZgwServer::~ZgwServer() {
  for (int i = 0; i < worker_num_; i++) {
    delete worker_store_[zgw_worker_thread_[i]];
    delete zgw_worker_thread_[i];
  }
  delete zgw_dispatch_thread_;
  delete conn_factory;

  LOG(INFO) << "ZgwServerThread " << pthread_self() << " exit!!!";
}

Status ZgwServer::Start() {
  Status s;
  LOG(INFO) << "Waiting for ZgwServerThread Init, About "<< worker_num_ * 10 << "s";
  for (int i = 0; i < worker_num_; i++) {
    s = InitWorderThread(zgw_worker_thread_[i], zgw_conf_->zp_meta_ip_ports);
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
    // DoTimingTask
    usleep(kZgwCronInterval);
  }

  return Status::OK();
}
