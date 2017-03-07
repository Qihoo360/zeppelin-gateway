#include <openssl/hmac.h>
#include "slash_string.h"
#include "slash_hash.h"

#include "zgw_auth.h"

bool ZgwAuth::Auth(const pink::HttpRequest *req, std::string secret_key) {
  /* e.g.
   *Authorization: AWS4-HMAC-SHA256 Credential=tC22yNVe9FJ9S0vs5OYx/20170306/us-east-1/s3/aws4_request, SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date, Signature=08e2daeba9b8bb552dc584559302f304684dcd31b058534675aa57d911be17ce
   */
  // Parse authorization string
  auto it = req->headers.find("authorization");
  if(it == req->headers.end() ||
     !ParseAuthStr(it->second)) {
    return false;
  }

  // Task 1: Create a Canonical Request
  std::string canonical_request = CreateCanonicalRequest(req);
  // Task 2: Create a String to Sign
  pink::HttpRequest *req_tmp = const_cast<pink::HttpRequest *>(req);
  std::string string_to_sign;
  string_to_sign.append(encryption_method_ + "\n");
  string_to_sign.append(req_tmp->headers["x-amz-date"] + "\n");
  string_to_sign.append(date_ + "/" + region_ + "/s3/aws4_request\n");
  string_to_sign.append(slash::sha256(canonical_request));
  // Task 3: Calculate Signature
  std::string date_key = HMAC_SHA256("AWS4" + secret_key, date_);
  std::string date_region_key = HMAC_SHA256(date_key, region_);
  std::string date_region_service_key = HMAC_SHA256(date_region_key, "s3");
  std::string signing_key = HMAC_SHA256(date_region_service_key, "aws4_request");

  std::string signature = HMAC_SHA256(signing_key, string_to_sign, false);
  if (signature != signature_) {
    return false;
  }
  return true;
}

bool ZgwAuth::ParseAuthStr(std::string auth_str) {
  std::vector<std::string> items;
  slash::StringSplit(auth_str, ' ', items);
  if (items.size() != 4) {
    return false;
  }
  encryption_method_ = items[0];
  date_ = items[1].substr(32, 8);
  region_ = items[1].substr(41, items[1].size() - 41 - 17);
  signed_headers_str_ = items[2].substr(14, items[2].size() - 14 - 1);
  slash::StringSplit(signed_headers_str_, ';', signed_headers_);
  signature_ = items[3].substr(10, items[3].size() - 10);
  return true;
}

std::string HMAC_SHA256(const std::string key, const std::string value, bool raw) {
  unsigned char *digest;
  digest = HMAC(EVP_sha256(), key.data(), key.size(),
                (unsigned char *)value.data(), value.size(), NULL, NULL);

  std::string res;
  if (raw) {
    for (int i = 0; i < 32; ++i) {
      res.append(1, digest[i]);
    }
    return res;
  }

  // Hex
  char buf[65] = {0};
  for (int i = 0; i < 32; ++i) {
    sprintf(buf + i * 2, "%02x", (unsigned int)digest[i]);
  }
  return std::string(buf);
}

static void hexchar(unsigned char c, unsigned char &hex1, unsigned char &hex2) {
    hex1 = c / 16;
    hex2 = c % 16;
    hex1 += hex1 <= 9 ? '0' : 'a' - 10;
    hex2 += hex2 <= 9 ? '0' : 'a' - 10;
}

std::string UrlEncode(std::string s) {
  const char *str = s.c_str();
  std::string v;
  v.clear();
  for (size_t i = 0, l = s.size(); i < l; i++) {
    char c = str[i];
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c == '_' || c == '-' || c == '~' || c == '.' || c == '/')) {
      v.push_back(c);
    } else if (c == ' ') {
      v.append("%20");
    } else {
      v.push_back('%');
      unsigned char d1, d2;
      hexchar(c, d1, d2);
      v.push_back(d1);
      v.push_back(d2);
    }
  }

  return v;
}

std::string ZgwAuth::CreateCanonicalRequest(const pink::HttpRequest *req) {
  std::string result;
  // <HTTPMethod>\n
  result.append(req->method + "\n");
  // <CanonicalURI>\n
  result.append(UrlEncode(req->path) + "\n");
  // <CanonicalQueryString>\n
  for (auto &q : req->query_params) {
    result.append(UrlEncode(q.first) + "=" + UrlEncode(q.second) + "&");
  }
  if (result.back() == '&') {
    result.resize(result.size() - 1); // delete last '&'
  }
  result.append("\n");
  // <CanonicalHeaders>\n
  pink::HttpRequest *req_tmp = const_cast<pink::HttpRequest *>(req);
  for (auto &q : signed_headers_) {
    result.append(slash::StringToLower(q) + ":" + req_tmp->headers[q] + "\n");
  }
  result.append("\n");
  
  result.append(signed_headers_str_ + "\n");
  result.append(slash::sha256(req->content));
  return result;
}

#ifdef UNIT_TEST
int main () {
  // std::cout << HMAC_SHA256("hello", "test", false) << std::endl;
  std::cout << urlencode("http://test.com/hello/wow/") << std::endl;

  return 0;
}
#endif
