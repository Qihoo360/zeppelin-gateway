#ifndef ZGW_SERVER_H
#define ZGW_SERVER_H

#include <string>
#include <map>
#include <pthread.h>

#include <glog/logging.h>
#include "pink/include/server_thread.h"
#include "slash/include/slash_status.h"
#include "slash/include/slash_mutex.h"

#include "src/zgwstore/zgw_store.h"
#include "src/zgw_s3_rest.h"
#include "src/zgw_const.h"
#include "src/zgw_admin_conn.h"

#include "src/zgw_config.h"

using slash::Status;

class MyThreadEnvHandle : public pink::ThreadEnvHandle {
 public:
  MyThreadEnvHandle() {
  }

  virtual ~MyThreadEnvHandle() {
    for (auto s : stores_) {
      delete s;
    }
  }

  virtual int SetEnv(void** env) const;

 private:
  mutable std::vector<zgwstore::ZgwStore*> stores_;
};

class ZgwServer {
 public:
  explicit ZgwServer();
  virtual ~ZgwServer();
  Status Start();

  uint64_t qps();
  void AddQueryNum();

  bool running() const {
    return !should_exit_.load();
  }

  void Exit();

 private:
  // Server related
	std::vector<std::string> zp_meta_ip_ports_;
  std::string ip_;
  std::atomic<bool> should_exit_;

  int worker_num_;
  ZgwConnFactory* conn_factory_;

  ZgwAdminConnFactory* admin_conn_factory_;
  pink::ServerThread* zgw_dispatch_thread_;
  pink::ServerThread* zgw_admin_thread_;

  uint64_t last_query_num_;
  uint64_t cur_query_num_;
  uint64_t last_time_us_;
  pthread_rwlock_t qps_lock_;
};

#endif
