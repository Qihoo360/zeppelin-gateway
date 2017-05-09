#ifndef ZGW_UTIL_H
#define ZGW_UTIL_H

#include <string>
#include <chrono>

#include <glog/logging.h>
#include "pink/include/http_conn.h"

extern void SplitBySecondSlash(const std::string& req_path,
                               std::string* field1, std::string* field2);
extern std::string http_nowtime(time_t t);
extern std::string md5(const std::string& content);

struct Timer {
  Timer(const char* msg)
      : msg_(msg) {
    start = std::chrono::system_clock::now();
  }

  Timer(const std::string& msg)
      : Timer(msg.c_str()) {
  }

  ~Timer() {
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    DLOG(INFO) << msg_ << " elapse " << diff.count() << " ms";
  }

  std::string msg_;
  std::chrono::system_clock::time_point start;
};

#endif
