#ifndef ZGW_UTIL_H
#define ZGW_UTIL_H

#include <string>
#include <chrono>

#include <glog/logging.h>
#include "pink/include/http_conn.h"

void ExtraBucketAndObject(const std::string& _path,
                          std::string* bucket_name, std::string* object_name);
std::string http_nowtime(time_t t);
std::string md5(const std::string& content);
void DumpHttpRequest(const pink::HttpRequest* req);

struct Timer {
  Timer(const char* msg)
      : msg_(msg) {
    start = std::chrono::system_clock::now();
  }

  ~Timer() {
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    DLOG(INFO) << msg_ << "elapse " << diff.count() << " ms";
  }

  std::string msg_;
  std::chrono::system_clock::time_point start;
};

#endif
