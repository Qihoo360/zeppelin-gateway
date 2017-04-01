#ifndef ZGW_AUTH_H
#define ZGW_AUTH_H

#include <iostream>
#include <string>

#include "pink/include/http_conn.h"

extern std::string UrlEncode(const std::string& s, bool encode_slash = false);
extern std::string UrlDecode(const std::string& url);

class ZgwAuth {
 public:
  ZgwAuth() {}

  bool ParseAuthInfo(const pink::HttpRequest* req, std::string* access_key);
  bool Auth(const pink::HttpRequest *req, const std::string& secret_key);

  std::string canonical_request() {
    return canonical_request_;
  }

 private:
  bool is_presign_url_;
  std::string encryption_method_;
  std::string date_;
  std::string iso_date_;
  std::string region_;
  std::vector<std::string> signed_headers_;
  std::string signed_headers_str_;
  std::string signature_;
  std::string access_key_;
  std::string canonical_request_;

  bool ParseAuthStr(const std::map<std::string, std::string>& headers);
  bool ParseQueryAuthStr(const std::map<std::string, std::string>& query_params);
  bool ParseCredential(const std::string& credential_str);
  std::string CreateCanonicalRequest(const pink::HttpRequest *req);
};

#endif
