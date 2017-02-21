#include "zgw_server.h"

#include <glog/logging.h>

ZgwServer::ZgwServer(const std::string zp_meta_ip_port,
                     const std::string& server_ip,
                     int server_port)
  : ip_(server_ip),
  port_(server_port),
  should_exit_(false) {
    worker_num_ = 4;
    for (int i = 0; i < worker_num_; i++) {
      zgw_worker_thread_[i] = new pink::WorkerThread<ZgwConn>();
    }
    std::set<std::string> ips;
    ips.insert(ip_);
    zgw_dispatch_thread_ = new pink::DispatchThread<ZgwConn>(
        ips,
        port_,
        worker_num_,
        zgw_worker_thread_,
        kDispatchCronInterval);
    zp_meta_ip_ports_.push_back(zp_meta_ip_port);
}

ZgwServer::~ZgwServer() {
  delete zgw_dispatch_thread_;
  for (int i = 0; i < worker_num_; i++) {
    delete zgw_worker_thread_[i];
  }

  LOG(INFO) << "ZgwServerThread " << pthread_self() << " exit!!!";
}

slash::Status ZgwServer::Start() {
  // Open ZgwStore
  libzgw::ZgwStore::Open(zp_meta_ip_ports_, &store_); 
  zgw_dispatch_thread_->StartThread();
  
  while (!should_exit_) {
    DoTimingTask();
    // int sleep_count = kZgwCronCount;
    // while (!should_exit_ && sleep_count-- > 0){
    usleep(kZgwCronInterval * 1000);
    // }
  }
  return slash::Status::OK();
}

void ZgwServer::DoTimingTask() {}
