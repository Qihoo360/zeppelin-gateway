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

  libzgw::ZgwStore* admin_store() {
    return admin_store_;
  }

  uint64_t qps();
  void AddQueryNum();

  libzgw::ZgwStore* GetWorkerStore(pink::Thread* worker);

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
  std::map<pink::Thread*, libzgw::ZgwStore*> worker_store_;
  ZgwConnFactory *conn_factory_;
  pink::Thread* zgw_worker_thread_[kMaxWorkerThread];

  int admin_port_;
  libzgw::ZgwStore* admin_store_;
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

  Status InitWorderThread(pink::Thread* worker,
                          std::vector<std::string> &zp_meta_ip_ports);
  Status InitAdminThread();
};

#endif
