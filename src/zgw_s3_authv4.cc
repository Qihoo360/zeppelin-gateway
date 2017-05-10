#include "src/zgw_s3_authv4.h"

#include <cctype>
#include <utility>
#include <map>
#include <vector>

#include <openssl/hmac.h>
#include "slash/include/slash_string.h"
#include "slash/include/slash_hash.h"
#include "slash/include/env.h"
#include "slash/include/slash_status.h"
#include "src/zgw_util.h"

static std::string HMAC_SHA256(const std::string key, const std::string value, bool raw = true);

// Expire after 5 minutes
//              access_key              user_name    secret_key 
static std::map<std::string, std::pair<std::string, std::string>> key_map;
static uint64_t latest_update_time;
static const uint64_t kFiveMinutesUs = 5 * 60 * 1e6;
static std::string last_access_key;

struct S3AuthV4::Rep {

  std::string user_name_;
  std::string access_key_;
  bool is_presign_url_;
  std::string encryption_method_;
  std::string date_;
  std::string iso_date_;
  std::string region_;
  std::vector<std::string> signed_headers_;
  std::string signed_headers_str_;
  std::string signature_;
  std::string signature_received_;
  std::string canonical_request_;

  void Clear();
  void UpdateKeyMap(zgwstore::ZgwStore* store);
  void CalcSignature(const std::string& secret_key);
  bool ParseHeaderAuthStr(const std::map<std::string, std::string>& headers);
  bool ParseQueryAuthStr(const std::map<std::string, std::string>& query_params);
  bool ParseCredential(const std::string& credential_str);
  void CreateCanonicalRequest(const pink::HttpRequest *req);
};

S3AuthV4::S3AuthV4() {
  rep_ = new Rep();
}

S3AuthV4::~S3AuthV4() {
  delete rep_;
}

void S3AuthV4::Initialize(const pink::HttpRequest* req, zgwstore::ZgwStore* store) {
  rep_->Clear();
  if (!rep_->ParseHeaderAuthStr(req->headers_) &&
      !rep_->ParseQueryAuthStr(req->query_params_)) {
    return;
  }
  // Task 1: Create a Canonical Request
  rep_->CreateCanonicalRequest(req);
  // Get secret key
  assert(!rep_->access_key_.empty());

  std::string secret_key;
  int retry = 2;
  while (retry--) {
    if (key_map.count(rep_->access_key_)) {
      rep_->user_name_.assign(key_map.at(rep_->access_key_).first);
      secret_key.assign(key_map.at(rep_->access_key_).second);
      break;
    }
    if (!key_map.empty() &&
        rep_->access_key_ == last_access_key) {
      if (slash::NowMicros() - latest_update_time > kFiveMinutesUs) {
        last_access_key.clear();
      }
      // Repeated invalid key
      return;
    }
    rep_->UpdateKeyMap(store);
    last_access_key = rep_->access_key_;
  }

  // Task 2-3
  rep_->CalcSignature(secret_key);
}

std::string S3AuthV4::ToString() {
  char buf[1024];
  sprintf(buf, "");
  return std::string(buf);
}

AuthErrorType S3AuthV4::TryAuth() {
  if (rep_->encryption_method_.find("AWS4-HMAC-SHA256") ==
      std::string::npos) {
    return kMaybeAuthV2;
  } else if (!key_map.count(rep_->access_key_)) {
    return kAccessKeyInvalid;
  } else if (rep_->signature_ != rep_->signature_received_) {
    return kSignatureNotMatch;
  }
  return kAuthSuccess;
}

std::string S3AuthV4::user_name() {
  return rep_->user_name_;
}

void S3AuthV4::Rep::UpdateKeyMap(zgwstore::ZgwStore* store) {
  std::vector<zgwstore::User> all_users;
  slash::Status s = store->ListUsers(&all_users);
  if (s.ok()) {
    for (auto& user : all_users) {
      for (auto& key_pair : user.key_pairs) {
        key_map.insert(std::make_pair(key_pair.first,
                       std::make_pair(user.display_name, key_pair.second)));
      }
    }
  }
}

void S3AuthV4::Rep::Clear() {
  user_name_.clear();
  access_key_.clear();
  is_presign_url_ = false;
  encryption_method_.clear();
  date_.clear();
  iso_date_.clear();
  region_.clear();
  signed_headers_.clear();
  signed_headers_str_.clear();
  signature_.clear();
  signature_received_.clear();
  access_key_.clear();
  canonical_request_.clear();
}

void S3AuthV4::Rep::CalcSignature(const std::string& secret_key) {
  // Task 2: Create a String to Sign
  std::string string_to_sign;
  string_to_sign.append(encryption_method_ + "\n");
  string_to_sign.append(iso_date_ + "\n");
  string_to_sign.append(date_ + "/" + region_ + "/s3/aws4_request\n");
  string_to_sign.append(slash::sha256(canonical_request_));
  // Task 3: Calculate Signature
  std::string date_key = HMAC_SHA256("AWS4" + secret_key, date_);
  std::string date_region_key = HMAC_SHA256(date_key, region_);
  std::string date_region_service_key = HMAC_SHA256(date_region_key, "s3");
  std::string signing_key = HMAC_SHA256(date_region_service_key, "aws4_request");

  signature_ = HMAC_SHA256(signing_key, string_to_sign, false);
}

bool S3AuthV4::Rep::ParseCredential(const std::string& credential_str) {
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

bool S3AuthV4::Rep::ParseQueryAuthStr(const std::map<std::string, std::string>& query_params) {
  if (!query_params.count("X-Amz-Algorithm") ||
      !query_params.count("X-Amz-Credential") ||
      !query_params.count("X-Amz-Date") ||
      !query_params.count("X-Amz-Signature") ||
      !query_params.count("X-Amz-SignedHeaders")) {
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

  signature_received_ = UrlDecode(query_params.at("X-Amz-Signature"));
  iso_date_ = UrlDecode(query_params.at("X-Amz-Date"));

  is_presign_url_ = true;
  return true;
}

bool S3AuthV4::Rep::ParseHeaderAuthStr(const std::map<std::string, std::string>& headers) {
  /* e.g.
   *Authorization: AWS4-HMAC-SHA256 Credential=tC22yNVe9FJ9S0vs5OYx/20170306/us-east-1/s3/aws4_request, SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date, Signature=08e2daeba9b8bb552dc584559302f304684dcd31b058534675aa57d911be17ce
   */
  if (!headers.count("authorization") ||
      !headers.count("x-amz-date")) {
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
  signature_received_ = signature.substr(10, 64);

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
    for (unsigned int i = 0; i < digest_len; ++i) {
      res.append(1, digest[i]);
    }
    return res;
  }

  // Hex
  char buf[65] = {0};
  for (unsigned int i = 0; i < digest_len; ++i) {
    sprintf(buf + i * 2, "%02x", (unsigned int)digest[i]);
  }
  delete[] digest;
  return std::string(buf);
}

void S3AuthV4::Rep::CreateCanonicalRequest(const pink::HttpRequest *req) {
  canonical_request_.clear();
  // <HTTPMethod>\n
  canonical_request_.append(req->method_ + "\n");
  // <CanonicalURI>\n
  std::string ori_path = UrlDecode(req->path_);
  canonical_request_.append(UrlEncode(ori_path) + "\n");
  // <CanonicalQueryString>\n
  for (auto &q : req->query_params_) {
    if (q.first.compare("X-Amz-Signature") == 0) {
      continue;
    }
    canonical_request_.append(UrlEncode(q.first, true) + "=" + UrlEncode(q.second, true) + "&");
  }
  if (!canonical_request_.empty() && canonical_request_.back() == '&') {
    canonical_request_.pop_back(); // delete last '&'
  }
  canonical_request_.append("\n");
  // <CanonicalHeaders>\n
  for (auto &q : signed_headers_) {
    if (req->headers_.count(q)) {
      canonical_request_.append(slash::StringToLower(q) + ":" + req->headers_.at(q) + "\n");
    }
  }
  canonical_request_.append("\n");
  
  std::string content_sha265;
  if (req->headers_.count("x-amz-content-sha256")) {
    content_sha265.assign(req->headers_.at("x-amz-content-sha256"));
  }
  canonical_request_.append(signed_headers_str_ + "\n");
  canonical_request_.append(is_presign_url_ ? "UNSIGNED-PAYLOAD" : content_sha265);
}
