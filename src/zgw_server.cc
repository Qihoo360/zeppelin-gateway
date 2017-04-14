#include "zgw_server.h"

#include <glog/logging.h>
#include "slash/include/slash_mutex.h"
#include "slash/include/env.h"

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

Status ZgwServer::InitAdminThread() {
  // Open ZgwStore
  Status s =
    libzgw::ZgwStore::Open(zgw_conf_->zp_meta_ip_ports, &admin_store_); 
  if (!s.ok()) {
    LOG(FATAL) << "Can not open ZgwStore: " << s.ToString();
    return s;
  }

  return Status::OK();
}

ZgwServer::ZgwServer(ZgwConfig *zgw_conf)
    : zgw_conf_(zgw_conf),
      ip_(zgw_conf->server_ip),
      should_exit_(false),
      worker_num_(zgw_conf->worker_num),
      port_(zgw_conf->server_port),
      admin_port_(zgw_conf->admin_port),
      last_query_num_(0),
      cur_query_num_(0),
      last_time_us_(0) {
  pthread_rwlock_init(&qps_lock_, NULL);
  last_time_us_ = slash::NowMicros();
  if (worker_num_ > kMaxWorkerThread) {
    LOG(WARNING) << "Exceed max worker thread num: " << kMaxWorkerThread;
    worker_num_ = kMaxWorkerThread;
  }
  conn_factory_ = new ZgwConnFactory();
  for (int i = 0; i < worker_num_; i++) {
    zgw_worker_thread_[i] = pink::NewWorkerThread(conn_factory_);
    zgw_worker_thread_[i]->set_thread_name("WorkerThread");
  }
  std::set<std::string> ips;
  ips.insert(ip_);
  zgw_dispatch_thread_ = pink::NewDispatchThread(ips, port_, worker_num_, zgw_worker_thread_);
  zgw_dispatch_thread_->set_thread_name("DispatchThread");

  admin_conn_factory_ = new AdminConnFactory();
  zgw_admin_thread_ = pink::NewHolyThread(admin_port_, admin_conn_factory_);
  zgw_admin_thread_->set_thread_name("AdminThread");

  buckets_list_ = new libzgw::ListMap(libzgw::ListMap::kBuckets);
  objects_list_ = new libzgw::ListMap(libzgw::ListMap::kObjects);
}

ZgwServer::~ZgwServer() {
  for (int i = 0; i < worker_num_; i++) {
    delete worker_store_[zgw_worker_thread_[i]];
    delete zgw_worker_thread_[i];
  }
  delete zgw_dispatch_thread_;
  delete zgw_admin_thread_;
  delete conn_factory_;
  delete admin_conn_factory_;

  LOG(INFO) << "ZgwServerThread " << pthread_self() << " exit!!!";
}

uint64_t ZgwServer::qps() {
  uint64_t cur_time_us = slash::NowMicros();
  slash::RWLock l(&qps_lock_, false);
  uint64_t qps = (cur_query_num_ - last_query_num_) * 1000000
    / (cur_time_us - last_time_us_ + 1);
  if (qps == 0) {
    cur_query_num_ = 0;
  }
  last_query_num_ = cur_query_num_;
  last_time_us_ = cur_time_us;
  return qps;
}

void ZgwServer::AddQueryNum() {
  slash::RWLock l(&qps_lock_, true);
  ++cur_query_num_;
}

void ZgwServer::Exit() {
  for (int i = 0; i < worker_num_; i++) {
    zgw_worker_thread_[i]->StopThread();
  }
  zgw_dispatch_thread_->StopThread();
  zgw_admin_thread_->StopThread();
  should_exit_.store(true);
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

  s = InitAdminThread();
  if (!s.ok()) {
    return s;
  }
  
  if (zgw_dispatch_thread_->StartThread() != 0) {
    return Status::Corruption("Launch DispatchThread failed");
  }
  if (zgw_admin_thread_->StartThread() != 0) {
    return Status::Corruption("Launch AdminThread failed");
  }
  LOG(INFO) << "ZgwServerThread Init Success!";

  while (running()) {
    // DoTimingTask
    slash::SleepForMicroseconds(kZgwCronInterval);
    qps();
  }

  return Status::OK();
}
