#ifndef ZGW_SERVER_H
#define ZGW_SERVER_H

#include <string>
j
#include "include/worker_thread.h"
#include "include/dispatch_thread.h"
#include "include/slash_status.h"
#include "include/slash_mutex.h"

#include "zgw_conf.h"


extern ZgwConf* g_zgw_conf;

class ZgwServer {
 public:
  explicit ZgwServer();
  virtual ~ZgwServer();
  Status Start();

  std::string local_ip() {
    return g_zgw_conf->local_ip();
  }
  int local_port() {
    return g_zgw_conf->local_port();
  }

  void Exit() {
    should_exit_ = true;
  }

 private:
  // Server related
  int worker_num_;
  pink::WorkerThread<ZgwHttpConn>* zgw_worker_thread_[kMaxWorkerThread];
  pink::DispatchThread<ZgwHttpConn> *zgw_dispatch_thread_;

  std::atomic<bool> should_exit_;

  void DoTimingTask();
};

#endif
