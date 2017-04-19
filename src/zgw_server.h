#ifndef ZGW_SERVER_H
#define ZGW_SERVER_H

#include <string>
#include <map>
#include <pthread.h>

#include <glog/logging.h>
#include "pink/include/server_thread.h"
#include "slash/include/slash_status.h"
#include "slash/include/slash_mutex.h"

#include "src/libzgw/zgw_store.h"
#include "src/libzgw/zgw_namelist.h"
#include "src/zgw_const.h"
#include "src/zgw_conn.h"
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
  mutable std::vector<libzgw::ZgwStore*> stores_;
};

class ZgwServer {
 public:
  explicit ZgwServer();
  virtual ~ZgwServer();
  Status Start();

  std::string local_ip() const {
    return ip_;
  }

  int local_port() const {
    return port_;
  }

  Status RefAndGetBucketList(libzgw::ZgwStore* store,
                              const std::string& access_key,
                              libzgw::NameList** buckets_name) {
    return buckets_list_->Ref(store, access_key, buckets_name);
  }

  Status RefAndGetObjectList(libzgw::ZgwStore* store,
                              const std::string& bucket_name,
                              libzgw::NameList** objects_name) {
    return objects_list_->Ref(store, bucket_name, objects_name);
  }

  Status UnrefBucketList(libzgw::ZgwStore* store, const std::string& access_key) {
    return buckets_list_->Unref(store, access_key);
  }

  Status UnrefObjectList(libzgw::ZgwStore* store, const std::string& bucket_name) {
    return buckets_list_->Unref(store, bucket_name);
  }

  void ObjectLock(const std::string& full_object_name) {
    object_mutex_.Lock(full_object_name);
  }

  void ObjectUnlock(const std::string& full_object_name) {
    object_mutex_.Unlock(full_object_name);
  }

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
  int port_;
  std::mutex worker_store_mutex_;
  ZgwConnFactory* conn_factory_;

  int admin_port_;
  AdminConnFactory* admin_conn_factory_;
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
