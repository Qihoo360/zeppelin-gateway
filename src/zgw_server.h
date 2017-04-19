#ifndef ZGW_SERVER_H
#define ZGW_SERVER_H

#include <string>
#include <map>
#include <pthread.h>

#include <glog/logging.h>
#include "pink/include/server_thread.h"
#include "slash/include/slash_status.h"
#include "slash/include/slash_mutex.h"

#include "libzgw/zgw_store.h"
#include "libzgw/zgw_namelist.h"
#include "zgw_const.h"
#include "zgw_conn.h"
#include "zgw_admin_conn.h"

#include "zgw_config.h"

using slash::Status;

class MyThreadEnvHandle : public pink::ThreadEnvHandle {
 public:
  MyThreadEnvHandle(const ZgwConfig* zgw_conf)
      : zgw_conf_(zgw_conf) {
  }

  virtual ~MyThreadEnvHandle() {
    for (auto s : stores_) {
      delete s;
    }
  }

  virtual int SetEnv(void** env) const;

 private:
  const ZgwConfig* zgw_conf_;
  mutable std::vector<libzgw::ZgwStore*> stores_;
};

class ZgwServer {
 public:
  explicit ZgwServer(ZgwConfig *zgw_conf);
  virtual ~ZgwServer();
  Status Start();

  std::string local_ip() const {
    return ip_;
  }

  int local_port() const {
    return port_;
  }

  libzgw::ListMap *objects_list() {
    return objects_list_;
  }

  libzgw::ListMap *buckets_list() {
    return buckets_list_;
  }

  slash::RecordMutex *object_mutex() {
    return &object_mutex_;
  }

  uint64_t qps();
  void AddQueryNum();

  bool running() const {
    return !should_exit_.load();
  }

  void Exit();

 private:
  ZgwConfig *zgw_conf_;
  // Server related
	std::vector<std::string> zp_meta_ip_ports_;
  std::string ip_;
  std::atomic<bool> should_exit_;

  int worker_num_;
  int port_;
  std::mutex worker_store_mutex_;
  ZgwConnFactory *conn_factory_;

  int admin_port_;
  AdminConnFactory *admin_conn_factory_;
  pink::ServerThread* zgw_dispatch_thread_;
  pink::ServerThread* zgw_admin_thread_;

  libzgw::ListMap* buckets_list_;
  libzgw::ListMap* objects_list_;
  slash::RecordMutex object_mutex_;

  uint64_t last_query_num_;
  uint64_t cur_query_num_;
  uint64_t last_time_us_;
  pthread_rwlock_t qps_lock_;
};

#endif
