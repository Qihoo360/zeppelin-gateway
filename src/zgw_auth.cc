#include "zgw_auth.h"

#include <cctype>

#include <openssl/hmac.h>
#include "slash/include/slash_string.h"
#include "slash/include/slash_hash.h"

static std::string HMAC_SHA256(const std::string key, const std::string value, bool raw = true);

bool ZgwAuth::ParseAuthInfo(const pink::HttpRequest* req, std::string* access_key) {
  // Parse authorization string
  if (!ParseAuthStr(req->headers) &&
      !ParseQueryAuthStr(req->query_params)) {
    return false;
  }
  access_key->assign(access_key_);
  return true;
}

bool ZgwAuth::Auth(const pink::HttpRequest* req, const std::string& secret_key) {
  // Task 1: Create a Canonical Request
  std::string canonical_request = CreateCanonicalRequest(req);
  canonical_request_ = canonical_request;
  // Task 2: Create a String to Sign
  pink::HttpRequest *req_tmp = const_cast<pink::HttpRequest *>(req);
  std::string string_to_sign;
  string_to_sign.append(encryption_method_ + "\n");
  string_to_sign.append(iso_date_ + "\n");
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

bool ZgwAuth::ParseCredential(const std::string& credential_str) {
  // tC22yNVe9FJ9S0vs5OYx/20170306/us-east-1/s3/aws4_request
  std::vector<std::string> items;
  slash::StringSplit(credential_str, '/', items);
  if (items.size() != 5) {
    return false;
  }

  access_key_ = items[0];
  date_ = items[1];
  region_ = items[2];
  return true;
}

bool ZgwAuth::ParseQueryAuthStr(const std::map<std::string, std::string>& query_params) {
  if (query_params.find("X-Amz-Algorithm") == query_params.end() ||
      query_params.find("X-Amz-Credential") == query_params.end() ||
      query_params.find("X-Amz-Date") == query_params.end() ||
      query_params.find("X-Amz-Signature") == query_params.end() ||
      query_params.find("X-Amz-SignedHeaders") == query_params.end()) {
    return false;
  }

  // AWS4-HMAC-SHA256
  encryption_method_ = UrlDecode(query_params.at("X-Amz-Algorithm"));
  // Credential=tC22yNVe9FJ9S0vs5OYx/20170306/us-east-1/s3/aws4_request
  if (!ParseCredential(UrlDecode(query_params.at("X-Amz-Credential")))) {
    return false;
  }

  // SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date
  signed_headers_str_ = UrlDecode(query_params.at("X-Amz-SignedHeaders"));
  slash::StringSplit(signed_headers_str_, ';', signed_headers_);

  signature_ = UrlDecode(query_params.at("X-Amz-Signature"));
  iso_date_ = UrlDecode(query_params.at("X-Amz-Date"));

  is_presign_url_ = true;
  return true;
}

bool ZgwAuth::ParseAuthStr(const std::map<std::string, std::string>& headers) {
  /* e.g.
   *Authorization: AWS4-HMAC-SHA256 Credential=tC22yNVe9FJ9S0vs5OYx/20170306/us-east-1/s3/aws4_request, SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date, Signature=08e2daeba9b8bb552dc584559302f304684dcd31b058534675aa57d911be17ce
   */
  if (headers.find("authorization") == headers.end() ||
      headers.find("x-amz-date") == headers.end()) {
    return false;
  }

  std::vector<std::string> items, items1;
  slash::StringSplit(headers.at("authorization"), ',', items);
  if (items.size() != 3) {
    return false;
  }

  slash::StringSplit(items[0], ' ', items1);
  if (items1.size() != 2) {
    return false;
  }
  // AWS4-HMAC-SHA256
  encryption_method_ = items1[0];
  // Credential=tC22yNVe9FJ9S0vs5OYx/20170306/us-east-1/s3/aws4_request
  size_t head_len = strlen("Credential=");
  if (items1[1].size() < head_len ||
      !ParseCredential(items1[1].substr(head_len))) {
    return false;
  }

  // SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date
  std::string signedheaders = slash::StringTrim(items[1]);
  head_len = strlen("SignedHeaders=");
  if (signedheaders.size() < head_len) {
    return false;
  }
  signed_headers_str_ = signedheaders.substr(head_len);
  slash::StringSplit(signed_headers_str_, ';', signed_headers_);
  
  // Signature=08e2daeba9b8bb552dc584559302f304684dcd31b058534675aa57d911be17ce
  std::string signature = slash::StringTrim(items[2]);
  if (signature.size() != 74) { // 10 + 64;
    return false;
  }
  signature_ = signature.substr(10, 64);

  // 20170328T093456Z
  iso_date_ = headers.at("x-amz-date");

  is_presign_url_ = false;
  return true;
}

std::string HMAC_SHA256(const std::string key, const std::string value, bool raw) {
  static unsigned int digest_len = 32;
  unsigned char* digest = new unsigned char[digest_len];

  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);

  HMAC_Init_ex(&ctx, key.data(), key.size(), EVP_sha256(), NULL);
  HMAC_Update(&ctx, (unsigned char*)value.data(), value.size());
  HMAC_Final(&ctx, digest, &digest_len);
  HMAC_CTX_cleanup(&ctx);

  std::string res;
  if (raw) {
    for (int i = 0; i < digest_len; ++i) {
      res.append(1, digest[i]);
    }
    return res;
  }

  // Hex
  char buf[65] = {0};
  for (int i = 0; i < digest_len; ++i) {
    sprintf(buf + i * 2, "%02x", (unsigned int)digest[i]);
  }
  delete[] digest;
  return std::string(buf);
}

inline void char2hex(unsigned char c, unsigned char &hex1, unsigned char &hex2) {
    hex1 = c / 16;
    hex2 = c % 16;
    hex1 += hex1 <= 9 ? '0' : 'a' - 10;
    hex2 += hex2 <= 9 ? '0' : 'a' - 10;
}

inline char hex2char(const std::string& hex) {
  assert(hex.size() == 2);
  char a = std::tolower(hex[0]);
  char b = std::tolower(hex[1]);
  a = a >= 'a' ? (10 + a - 'a') : (a - '0');
  b = b >= 'a' ? (10 + b - 'a') : (b - '0');
  return (a * 16 + b);
}

std::string UrlDecode(const std::string& url) {
  std::string v;
  size_t i = 0;
  while (i < url.size()) {
    if (url[i] == '%') {
      if (i + 3 > url.size()) {
        break;
      }
      char c = hex2char(url.substr(i + 1, 2));
      v.push_back(c);
      i += 3;
    } else {
      v.push_back(url[i++]);
    }
  }
  return v;
}

std::string UrlEncode(const std::string& s, bool encode_slash) {
  const char *str = s.c_str();
  std::string v;
  v.clear();
  for (size_t i = 0, l = s.size(); i < l; i++) {
    char c = str[i];
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c == '_' || c == '-' || c == '~' || c == '.')) {
      v.push_back(c);
    } else if (c == '/') {
      if (encode_slash) {
        v.append("%2F");
      } else {
        v.push_back(c);
      }
    } else if (c == ' ') {
      v.append("%20");
    } else {
      v.push_back('%');
      unsigned char d1, d2;
      char2hex(c, d1, d2);
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
  std::string a = UrlDecode(req->path);
  result.append(UrlEncode(a) + "\n");
  // <CanonicalQueryString>\n
  for (auto &q : req->query_params) {
    if (q.first.compare("X-Amz-Signature") == 0) {
      continue;
    }
    result.append(UrlEncode(q.first, true) + "=" + UrlEncode(q.second, true) + "&");
  }
  if (!result.empty() && result.back() == '&') {
    result.pop_back(); // delete last '&'
  }
  result.append("\n");
  // <CanonicalHeaders>\n
  pink::HttpRequest *req_tmp = const_cast<pink::HttpRequest *>(req);
  for (auto &q : signed_headers_) {
    result.append(slash::StringToLower(q) + ":" + req_tmp->headers[q] + "\n");
  }
  result.append("\n");
  
  std::string content_sha265 = req_tmp->headers["x-amz-content-sha256"];
  result.append(signed_headers_str_ + "\n");
  result.append(is_presign_url_ ? "UNSIGNED-PAYLOAD" : content_sha265);
  return result;
}

#ifdef UNIT_TEST
int main () {
  // std::cout << HMAC_SHA256("hello", "test", false) << std::endl;
  std::cout << urlencode("http://test.com/hello/wow/") << std::endl;

  return 0;
}
#endif
