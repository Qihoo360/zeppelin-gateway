#ifndef ZGW_SERVER_H
#define ZGW_SERVER_H

#include <string>
#include <map>

#include <glog/logging.h>
#include "pink/include/server_thread.h"
#include "slash/include/slash_status.h"
#include "slash/include/slash_mutex.h"

#include "libzgw/zgw_store.h"
#include "libzgw/zgw_namelist.h"
#include "zgw_const.h"
#include "zgw_conn.h"
#include "zgw_config.h"

using slash::Status;

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

  slash::RecordMutex *object_mutex() {
    return &object_mutex_;
  }

  Status InitWorderThread(pink::Thread* worker,
                          std::vector<std::string> &zp_meta_ip_ports);

  libzgw::ZgwStore* GetWorkerStore(pink::Thread* worker);

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
  std::mutex worker_store_mutex_;
  std::map<pink::Thread*, libzgw::ZgwStore*> worker_store_;
  ZgwConnFactory *conn_factory;
  pink::Thread* zgw_worker_thread_[kMaxWorkerThread];
  pink::ServerThread* zgw_dispatch_thread_;

  libzgw::ListMap* buckets_list_;
  libzgw::ListMap* objects_list_;
  slash::RecordMutex object_mutex_;
};

#endif
