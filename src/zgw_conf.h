#ifndef ZGW_CONF_H_
#define ZGW_CONF_H_

#include <string>
#include <vector>

#include "include/base_conf.h"
#include "include/slash_string.h"
#include "include/slash_mutex.h"

#include "zgw_const.h"


class ZgwConf {
 public:
  ZgwConf();
  ~ZgwConf();

  void Dump() const;

  int Load(const std::string& path);

  std::string local_ip() {
    RWLock l(&rwlock_, false);
    return local_ip_;
  }

  int local_port() {
    RWLock l(&rwlock_, false);
    return local_port_;
  }

  int64_t timeout() {
    RWLock l(&rwlock_, false);
    return timeout_;
  }

  std::string log_path() {
    RWLock l(&rwlock_, false);
    return log_path_;
  }

  bool daemonize() {
    RWLock l(&rwlock_, false);
    return daemonize_;
  }

  std::string pid_file() {
    RWLock l(&rwlock_, false);
    return pid_file_;
  }

  std::string lock_file() {
    RWLock l(&rwlock_, false);
    return lock_file_;
  }

 private:
  // copy disallowded
  ZgwConf(const ZgwConf& options);

  std::string local_ip_;
  int local_port_;
  int64_t timeout_;

  std::string log_path_;
  bool daemonize_;
  std::string pid_file_;
  std::string lock_file_;
  pthread_rwlock_t rwlock_;
};

#endif
