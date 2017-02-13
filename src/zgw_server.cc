#include "zgw_server.h"

#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include "zgw_worker_thread.h"
#include "zgw_dispatch_thread.h"


ZgwServer::ZgwServer()
  : should_exit_(false) {
    worker_num_ = 4;
    for (int i = 0; i < worker_num_; i++) {
      zgw_worker_thread_[i] = new pink::WorkerThread<ZgwHttpConn>();
    }
    zgw_dispatch_thread_ = new pink::DispatchThread<ZgwHttpConn>(
        zgw_conf->local_ip(),
        zgw_conf->local_port(),
        worker_num_,
        zgw_worker_thread_,
        kDispatchCronInterval);
  }

ZgwServer::~ZgwServer() {
  delete zgw_dispatch_thread_;
  for (int i = 0; i < worker_num_; i++) {
    delete zgw_worker_thread_[i];
  }

  LOG(INFO) << "ZgwServerThread " << pthread_self() << " exit!!!";
}

Status ZgwServer::Start() {
  zgw_dispatch_thread_->StartThread();
  
  while (!should_exit_) {
    DoTimingTask();
    int sleep_count = kNodeCronWaitCount;
    while (!should_exit_ && sleep_count-- > 0){
      usleep(kNodeCronInterval * 1000);
    }
  }
  return Status::OK();
}

void ZgwServer::DoTimingTask() {
}
