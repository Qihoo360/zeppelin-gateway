#ifndef ZGW_SERVER_H
#define ZGW_SERVER_H

#include <string>
#include "include/worker_thread.h"
#include "include/dispatch_thread.h"
#include "include/slash_status.h"
#include "include/slash_mutex.h"
#include <glog/logging.h>

#include "zgw_const.h"
#include "zgw_conn.h"

class ZgwServer {
 public:
  explicit ZgwServer(const std::string& ip, int port);
  virtual ~ZgwServer();
  slash::Status Start();

  std::string local_ip() {
    return ip_;
  }
  int local_port() {
    return port_;
  }
  void Exit() {
    should_exit_ = true;
  }

 private:
  // Server related
  std::string ip_;
  int port_;
  std::atomic<bool> should_exit_;
  int worker_num_;
  pink::WorkerThread<ZgwConn>* zgw_worker_thread_[kMaxWorkerThread];
  pink::DispatchThread<ZgwConn> *zgw_dispatch_thread_;

  void DoTimingTask();
};

#endif
