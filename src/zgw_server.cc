#include "src/zgw_server.h"

#include <atomic>

#include <glog/logging.h>
#include "slash/include/slash_mutex.h"
#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_command.h"
#include "src/zgw_monitor.h"

extern ZgwConfig* g_zgw_conf;
extern ZgwMonitor* g_zgw_monitor;

static std::string LockName() {
  static std::atomic<int> thread_seq_;
  return hostname() +
    std::to_string(g_zgw_conf->server_port) +
    std::to_string(thread_seq_++);
}

int ZgwServerHandle::CreateWorkerSpecificData(void** data) const {
  zgwstore::ZgwStore* store;
  Status s = zgwstore::ZgwStore::Open(g_zgw_conf->zp_meta_ip_ports,
                                      g_zgw_conf->redis_ip_port,
                                      g_zgw_conf->zp_table_name,
                                      LockName(), kZgwRedisLockTTL,
                                      g_zgw_conf->redis_passwd,
                                      &store);
  if (!s.ok()) {
    LOG(FATAL) << "Can not open ZgwStore: " << s.ToString();
    return -1;
  }
  *data = reinterpret_cast<void*>(store);

  return 0;
}

int ZgwServerHandle::DeleteWorkerSpecificData(void* data) const {
  delete reinterpret_cast<zgwstore::ZgwStore*>(data);
  return 0;
}

ZgwServer::ZgwServer()
    : should_exit_(false),
      worker_num_(g_zgw_conf->worker_num) {
  if (worker_num_ > kMaxWorkerThread) {
    LOG(WARNING) << "Exceed max worker thread num: " << kMaxWorkerThread;
    worker_num_ = kMaxWorkerThread;
  }

  zgw_dispatch_thread_ = pink::NewDispatchThread(g_zgw_conf->server_ip,
                                                 g_zgw_conf->server_port,
                                                 worker_num_, &conn_factory_,
                                                 0, 1000, &server_handle_);
  zgw_dispatch_thread_->set_thread_name("DispatchThread");

  zgw_admin_thread_ = pink::NewHolyThread(g_zgw_conf->server_ip,
                                          g_zgw_conf->admin_port,
                                          &admin_conn_factory_, 0,
                                          &server_handle_);
  zgw_admin_thread_->set_thread_name("AdminThread");
}

ZgwServer::~ZgwServer() {
  delete zgw_dispatch_thread_;
  delete zgw_admin_thread_;
  delete store_for_gc_;

  LOG(INFO) << "ZgwServerThread exit!!!";
}

void ZgwServer::Exit() {
  int ret = zgw_dispatch_thread_->StopThread();
  if (ret != 0) {
    LOG(WARNING) << "Stop dispath thread failed";
  } else {
    LOG(INFO) << "DispatchThread Exit";
  }
  ret = zgw_admin_thread_->StopThread();
  if (ret != 0) {
    LOG(WARNING) << "Stop admin thread failed";
  } else {
    LOG(INFO) << "AdminThread Exit";
  }
  ret = store_gc_thread_.StopThread();
  if (ret != 0) {
    LOG(WARNING) << "Stop GCThread failed";
  } else {
    LOG(INFO) << "GCThread Exit";
  }
  should_exit_.store(true);
}

Status ZgwServer::Start() {
  Status s;
  LOG(INFO) << "Waiting for ZgwServerThread Init...";

  if (g_zgw_conf->enable_security) {
    if (zgw_dispatch_thread_->EnableSecurity(g_zgw_conf->cert_file,
                                             g_zgw_conf->key_file) != 0) {
      return Status::Corruption("Enable Security failed, maybe wrong cert or key");
    }
  }
  if (zgw_dispatch_thread_->StartThread() != 0) {
    return Status::Corruption("Launch DispatchThread failed");
  }
  if (zgw_admin_thread_->StartThread() != 0) {
    return Status::Corruption("Launch AdminThread failed");
  }
  // Open new store ptr for gc thread
  s = zgwstore::ZgwStore::Open(g_zgw_conf->zp_meta_ip_ports,
                               g_zgw_conf->redis_ip_port,
                               g_zgw_conf->zp_table_name,
                               LockName(), kZgwRedisLockTTL,
                               g_zgw_conf->redis_passwd,
                               &store_for_gc_);
  store_gc_thread_.StartThread(store_for_gc_);

  LOG(INFO) << "ZgwServerThread Init Success!";

  while (running()) {
    // DoTimingTask
    slash::SleepForMicroseconds(kZgwCronInterval);
    g_zgw_monitor->UpdateAndGetQPS();
  }

  return Status::OK();
}
