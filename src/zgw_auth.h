#ifndef ZGW_AUTH_H
#define ZGW_AUTH_H

#include <iostream>
#include <string>

#include "include/http_conn.h"

std::string HMAC_SHA256(const std::string key, const std::string value, bool raw = true);
extern std::string UrlEncode(const std::string& s);
extern std::string UrlDecode(const std::string& url);

class ZgwAuth {
 public:
  ZgwAuth() {}

  bool Auth(const pink::HttpRequest *req, std::string secret_key);

 private:
  std::string encryption_method_;
  std::string date_;
  std::string region_;
  std::vector<std::string> signed_headers_;
  std::string signed_headers_str_;
  std::string signature_;

  bool ParseAuthStr(std::string auth_str);
  std::string CreateCanonicalRequest(const pink::HttpRequest *req);
};

#endif
