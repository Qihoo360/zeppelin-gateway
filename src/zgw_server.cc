#include "src/zgw_server.h"

#include <glog/logging.h>
#include "slash/include/slash_mutex.h"
#include "slash/include/env.h"

extern ZgwConfig* g_zgw_conf;

int MyThreadEnvHandle::SetEnv(void** env) const {
  zgwstore::ZgwStore* store;
  uint64_t now = slash::NowMicros();
  char buf[100] = {0};
  gethostname(buf, 100);
  std::string lock_name = std::string(buf) + std::to_string(g_zgw_conf->server_port);
  Status s = zgwstore::ZgwStore::Open(g_zgw_conf->zp_meta_ip_ports,
                                      g_zgw_conf->redis_ip_port,
                                      g_zgw_conf->zp_table_name,
                                      lock_name, kZgwRedisLockTTL,
                                      &store);
  if (!s.ok()) {
    LOG(FATAL) << "Can not open ZgwStore: " << s.ToString();
    return -1;
  }
  *env = static_cast<void*>(store);
  stores_.push_back(store);
  return 0;
}

static MyThreadEnvHandle env_handle;

ZgwServer::ZgwServer()
    : should_exit_(false),
      worker_num_(g_zgw_conf->worker_num),
      last_query_num_(0),
      cur_query_num_(0),
      last_time_us_(0) {
  pthread_rwlock_init(&qps_lock_, NULL);
  last_time_us_ = slash::NowMicros();
  if (worker_num_ > kMaxWorkerThread) {
    LOG(WARNING) << "Exceed max worker thread num: " << kMaxWorkerThread;
    worker_num_ = kMaxWorkerThread;
  }

  conn_factory_ = new ZgwConnFactory();
  std::set<std::string> ips;
  ips.insert(g_zgw_conf->server_ip);
  zgw_dispatch_thread_ = pink::NewDispatchThread(ips, g_zgw_conf->server_port,
                                                 worker_num_, conn_factory_,
                                                 0, nullptr, &env_handle);
  zgw_dispatch_thread_->set_thread_name("DispatchThread");

  admin_conn_factory_ = new ZgwAdminConnFactory();
  zgw_admin_thread_ = pink::NewHolyThread(g_zgw_conf->admin_port,
                                          admin_conn_factory_,
                                          0, nullptr, &env_handle);
  zgw_admin_thread_->set_thread_name("AdminThread");
}

ZgwServer::~ZgwServer() {
  delete zgw_dispatch_thread_;
  delete zgw_admin_thread_;
  delete conn_factory_;
  delete admin_conn_factory_;

  LOG(INFO) << "ZgwServerThread exit!!!";
}

uint64_t ZgwServer::qps() {
  uint64_t cur_time_us = slash::NowMicros();
  slash::RWLock l(&qps_lock_, false);
  uint64_t qps = (cur_query_num_ - last_query_num_) * 1000000
    / (cur_time_us - last_time_us_ + 1);
  if (qps == 0) {
    cur_query_num_ = 0;
  }
  last_query_num_ = cur_query_num_;
  last_time_us_ = cur_time_us;
  return qps;
}

void ZgwServer::AddQueryNum() {
  slash::RWLock l(&qps_lock_, true);
  ++cur_query_num_;
}

void ZgwServer::Exit() {
  zgw_dispatch_thread_->StopThread();
  zgw_admin_thread_->StopThread();
  should_exit_.store(true);
  DestroyCmdTable();
}

Status ZgwServer::Start() {
  Status s;
  LOG(INFO) << "Waiting for ZgwServerThread Init, maybe "<< worker_num_ * 10 << "s";

  InitCmdTable();
  
  if (zgw_dispatch_thread_->StartThread() != 0) {
    return Status::Corruption("Launch DispatchThread failed");
  }
  if (zgw_admin_thread_->StartThread() != 0) {
    return Status::Corruption("Launch AdminThread failed");
  }

  LOG(INFO) << "ZgwServerThread Init Success!";

  while (running()) {
    // DoTimingTask
    slash::SleepForMicroseconds(kZgwCronInterval);
    qps();
  }

  return Status::OK();
}
