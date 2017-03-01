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
#include "zgw_config.h"
#include "libzgw/zgw_store.h"
#include "libzgw/zgw_namelist.h"

using slash::Status;

class ZgwWorkerThread : public pink::WorkerThread<ZgwConn> {
 public:
  libzgw::ZgwStore* GetStore() {
    return store_;
  }
  Status Init(std::vector<std::string> &zp_meta_ip_ports);

 private:
  libzgw::ZgwStore* store_;
};

class ZgwServer {
 public:
  explicit ZgwServer(ZgwConfig *zgw_conf);
  virtual ~ZgwServer();
  Status Start();

  std::string local_ip() {
    return ip_;
  }
  int local_port() {
    return port_;
  }
  libzgw::ListMap *objects_list() {
    return objects_list_;
  }
  libzgw::ListMap *buckets_list() {
    return buckets_list_;
  }
  void Exit() {
    should_exit_ = true;
  }

 private:
  ZgwConfig *zgw_conf_;
  // Server related
	std::vector<std::string> zp_meta_ip_ports_;
  std::string ip_;
  int port_;
  std::atomic<bool> should_exit_;
  int worker_num_;
  ZgwWorkerThread* zgw_worker_thread_[kMaxWorkerThread];
  pink::DispatchThread<ZgwConn> *zgw_dispatch_thread_;
  libzgw::ListMap *buckets_list_;
  libzgw::ListMap *objects_list_;

  void DoTimingTask();
};

#endif
