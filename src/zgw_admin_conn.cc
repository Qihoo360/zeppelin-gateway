#include "src/zgw_admin_conn.h"

#include <string>
#include <vector>

#include "slash/include/slash_hash.h"
#include "src/zgw_util.h"
#include "src/zgwstore/zgw_define.h"
#include "src/zgwstore/zgw_store.h"

bool ZgwAdminHandles::ReqHeadersHandle(const pink::HttpRequest* req) {
  Initialize();

  SplitBySecondSlash(req->path_, &command_, &params_);

  if (req->method_ == "PUT" &&
      command_ == kAddUser) {
    zgwstore::User new_user;
    new_user.display_name = params_;
    new_user.user_id = slash::sha256(params_);
    auto key_pair = GenerateKeyPair();
    new_user.key_pairs.insert(key_pair);
    Status s = store_->AddUser(new_user);
    if (!s.ok()) {
      http_ret_code_ = 409; // Confict ?
      result_.assign(s.ToString());
    } else {
      http_ret_code_ = 200;
      result_.assign(key_pair.first + "\r\n" + key_pair.second + "\r\n");
    }
  } else if (req->method_ == "GET" &&
             command_ == kListUsers) {
    std::vector<zgwstore::User> all_users;
    store_->ListUsers(all_users);
    http_ret_code_ = 200;
    for (auto& user : all_users) {
      result_ += user.display_name + "\r\n";
      for (auto& key_pair : user.key_pairs) {
        result_ += key_pair.first + "\r\n";
        result_ += key_pair.second + "\r\n";
      }
      result_ += "\r\n";
    }
    http_ret_code_ = 200;
  } else if (req->method_ == "GET" &&
             command_ == kGetStatus) {
    http_ret_code_ = 200;
    result_ = GetZgwStatus();
  } else {
    http_ret_code_ = 501;
    result_ = ":(";
  }
}

void ZgwAdminHandles::RespHeaderHandle(pink::HttpResponse* resp) {
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(result_.size());
}

int ZgwAdminHandles::RespBodyPartHandle(char* buf, size_t max_size) {
  if (max_size < result_.size()) {
    memcpy(buf, result_.data(), max_size);
    result_.assign(result_.substr(max_size));
  } else {
    memcpy(buf, result_.data(), result_.size());
  }

  return std::min(max_size, result_.size());
}

std::string ZgwAdminHandles::GetZgwStatus() {
  return "";
}

void ZgwAdminHandles::Initialize() {
  command_.clear();
  params_.clear();
  result_.clear();
  http_ret_code_ = 200;
  store_ = static_cast<zgwstore::ZgwStore*>(thread_ptr_->get_private());
}

static std::string GenRandomStr(int width) {
  timeval now;
  gettimeofday(&now, NULL);
  srand(now.tv_usec);
  std::string key;
  char x;
  for (int i = 0; i < width; ++i) {
    do {
      x = static_cast<char>(rand());
    } while (x < '0' ||
             (x > '9' && x < 'A') ||
             (x > 'Z' && x < 'a') ||
             x > 'z');
    key.push_back(x);
  }
  return key;
}

std::pair<std::string, std::string> ZgwAdminHandles::GenerateKeyPair() {
  std::string key1, key2;
  key1 = GenRandomStr(20);
  key2 = GenRandomStr(40);
  return std::make_pair(key1, key2);
}
