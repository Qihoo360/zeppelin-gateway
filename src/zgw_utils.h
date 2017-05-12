#ifndef ZGW_UTIL_H
#define ZGW_UTIL_H

#include <string>
#include <chrono>

#include <openssl/md5.h>
#include <glog/logging.h>
#include "pink/include/http_conn.h"

extern void SplitBySecondSlash(const std::string& req_path,
                               std::string* field1, std::string* field2);
extern std::string http_nowtime(uint64_t nowmicros);
extern std::string iso8601_time(uint64_t nowmicros);

extern std::string md5(const std::string& content);
extern std::string UrlEncode(const std::string& s, bool encode_slash = false);
extern std::string UrlDecode(const std::string& url);

class MD5Ctx {
 public:
  void Init() {
    MD5_Init(&md5_ctx_);
    memset(buf_, 0, 33);
    memset(md5_, 0, 16);
  }
  void Update(const std::string& data) {
    MD5_Update(&md5_ctx_, data.c_str(), data.size());
  }
  void Update(const char* data, size_t data_size) {
    MD5_Update(&md5_ctx_, data, data_size);
  }
  std::string ToString() {
    MD5_Final(md5_, &md5_ctx_);
    for (int i = 0; i < 16; i++) {
      sprintf(buf_ + i * 2, "%02x", md5_[i]);
    }
    return std::string(buf_);
  }
 private:
  MD5_CTX md5_ctx_;
  char buf_[33];
  unsigned char md5_[16];
};

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
