#include "src/zgw_admin_conn.h"

#include <string>
#include <vector>

#include "slash/include/slash_hash.h"
#include "src/zgwstore/zgw_define.h"
#include "src/zgwstore/zgw_store.h"
#include "src/s3_cmds/zgw_s3_command.h"
#include "src/zgw_monitor.h"
#include "src/zgw_utils.h"
#include "src/zgw_const.h"
#include "src/zgw_config.h"

extern ZgwMonitor* g_zgw_monitor;
extern ZgwConfig* g_zgw_conf;

static const char* S3CommandsToString(S3Commands cmd_type);

static const std::string kAccessKey = "access_key";
static const std::string kSecretKey = "secret_key";

bool ZgwAdminHandles::HandleRequest(const pink::HTTPRequest* req) {
  Initialize();

  SplitBySecondSlash(req->path(), &command_, &params_);

  const int BUFSIZE = 2048;
  char buf[BUFSIZE];

  const std::map<std::string, std::string>& query_params = req->query_params();
  if (query_params.find("auth") == query_params.end()) {
    http_ret_code_ = 400;
    snprintf(buf, BUFSIZE,
             "{\"errno\": \"400\", \"errmsg\": \"InvalidArgument: auth\"}");
    result_.assign(buf);
    return false;  // Meaning needn't reply
  }
  if (query_params.at("auth") != g_zgw_conf->admin_auth) {
    http_ret_code_ = 403;
    snprintf(buf, BUFSIZE,
             "{\"errno\": \"403\", \"errmsg\": \"Administrator authorization failed!\"}");
    result_.assign(buf);
    return false;  // Meaning needn't reply
  }


  if (req->method() == "PUT" &&
      command_ == kAddUser) {
    zgwstore::User new_user;
    new_user.display_name = params_;
    new_user.user_id = slash::sha256(params_);
    std::pair<std::string, std::string> key_pair;

    if (query_params.find(kAccessKey) != query_params.end() &&
        query_params.find(kSecretKey) != query_params.end()) {
      key_pair = std::make_pair(query_params.at(kAccessKey),
                                query_params.at(kSecretKey));
    } else {
      key_pair = GenerateKeyPair();
    }

    new_user.key_pairs.insert(key_pair);
    Status s = store_->AddUser(new_user);
    if (!s.ok()) {
      http_ret_code_ = 409; // Confict ?
      snprintf(buf, BUFSIZE, "{\"errno\": \"%d\", \"errmsg\": \"%s\"}",
               http_ret_code_, s.ToString().c_str());
      result_.assign(buf);
    } else {
      http_ret_code_ = 200;
      snprintf(buf, BUFSIZE, "{\"errno\": \"%d\", \"errmsg\": \"\", "
               "\"access_key\": \"%s\", \"secret_key\": \"%s\"}",
               0, key_pair.first.c_str(), key_pair.second.c_str());
      result_.assign(buf);
    }
  } else if (req->method() == "PUT" &&
             command_ == kAddToken) {
    const std::string& user_name = params_;
    std::pair<std::string, std::string> key_pair;
    if (query_params.find(kAccessKey) != query_params.end() &&
        query_params.find(kSecretKey) != query_params.end()) {
      key_pair = std::make_pair(query_params.at(kAccessKey),
                                query_params.at(kSecretKey));
    } else {
      key_pair = GenerateKeyPair();
    }
    Status s = store_->AddUserToken(user_name, key_pair.first, key_pair.second);
    if (!s.ok()) {
      http_ret_code_ = 500;
      snprintf(buf, BUFSIZE, "{\"errno\": \"%d\", \"errmsg\": \"%s\"}",
               http_ret_code_, s.ToString().c_str());
      result_.assign(buf);
    } else {
      http_ret_code_ = 200;
      snprintf(buf, BUFSIZE, "{\"errno\": \"%d\", \"errmsg\": \"\","
               "\"access_key\": \"%s\", \"secret_key\": \"%s\"}",
               0, key_pair.first.c_str(), key_pair.second.c_str());
      result_.assign(buf);
    }
  } else if (req->method() == "DELETE" &&
             command_ == kDelToken) {
    const std::string& user_name = params_;
    if (query_params.find(kAccessKey) == query_params.end()) {
      http_ret_code_ = 400;
      snprintf(buf, BUFSIZE,
               "{\"errno\": \"400\", \"errmsg\": \"InvalidArgument: access_key\"}");
      result_.assign(buf);
    } else {
      Status s = store_->DelUserToken(user_name, query_params.at(kAccessKey));
      if (!s.ok()) {
        http_ret_code_ = 500;
        snprintf(buf, BUFSIZE, "{\"errno\": \"500\", \"errmsg\": \"%s\"}",
                 s.ToString().c_str());
        result_.assign(buf);
      } else {
        http_ret_code_ = 200;
        snprintf(buf, BUFSIZE, "{\"errno\": \"0\", \"errmsg\": \"\"}");
        result_.assign(buf);
      }
    }
  } else if (req->method() == "GET" &&
             command_ == kListUsers) {
    std::vector<zgwstore::User> all_users;
    Status s = store_->ListUsers(&all_users);
    if (!s.ok()) {
      snprintf(buf, BUFSIZE, "{\"errno\": \"500\", \"errmsg\": \"%s\"}",
               s.ToString().c_str());
      http_ret_code_ = 500;
    } else {
      std::string users_str;
      for (auto& user : all_users) {
        std::string key_pairs_str;
        for (auto& key_pair : user.key_pairs) {
          char kbuf[128];
          snprintf(kbuf, 128, "{\"access_key\": \"%s\", \"secret_key\": \"%s\"}",
                   key_pair.first.c_str(), key_pair.second.c_str());
          key_pairs_str.append(kbuf);
          key_pairs_str.push_back(',');
        }
        if (!key_pairs_str.empty()) {
          key_pairs_str.pop_back();
        }

        char user_buf[128];
        snprintf(user_buf, 128, "{\"username\": \"%s\", \"key_pairs\": [",
                 user.display_name.c_str());
        users_str.append(user_buf);
        users_str.append(key_pairs_str);
        users_str.append("]},");
      }
      if (!users_str.empty()) {
        users_str.pop_back();  // remove last ','
      }

      result_.assign("{\"errno\": \"0\", \"errmsg\": \"\", \"users\": [");
      result_.append(users_str);
      result_.append("]}");
      http_ret_code_ = 200;
    }
  } else if (req->method() == "GET" &&
             command_ == kGetStatus) {
    http_ret_code_ = 200;
    bool force = req->query_params().count("force");
    result_ = GetZgwStatus(force);
  } else if (req->method() == "GET" &&
             command_ == kGetVersion) {
    http_ret_code_ = 200;
    snprintf(buf, BUFSIZE, "{\"errno\": \"0\", \"errmsg\": \"\", "
             "\"version\": \"%s\", \"compiledate\": \"%s\"}",
             kZgwVersion.c_str(), kZgwCompileDate.c_str());
    result_.assign(buf);
  } else {
    http_ret_code_ = 501;
    snprintf(buf, BUFSIZE, "{\"errno\": \"501\", \"errmsg\": \"Unimplement\"}");
    result_.assign(buf);
  }

  // Needn't reply
  return false;
}

void ZgwAdminHandles::PrepareResponse(pink::HTTPResponse* resp) {
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(result_.size());
}

int ZgwAdminHandles::WriteResponseBody(char* buf, size_t max_size) {
  if (max_size < result_.size()) {
    memcpy(buf, result_.data(), max_size);
    result_.assign(result_.substr(max_size));
  } else {
    memcpy(buf, result_.data(), result_.size());
  }

  return std::min(max_size, result_.size());
}

std::string ZgwAdminHandles::GetZgwStatus(bool force) {
  const size_t buf_size = 1024 * 1024;
  char buf[buf_size]; // 1MB
  const char* format = "\
  {\
      \"qps\": \"%lu\",\
      \"cluster_traffic\": \"%lu\",\
      \"request_count\": \"%lu\",\
      \"failed_request\": \"%lu\",\
      \"avg_upload_part_time\": \"%lu\",\
      \"auth_failed\": \"%lu\",\
      \"buckets_info\": [";

  snprintf(buf, buf_size, format,
           g_zgw_monitor->UpdateAndGetQPS(),
           g_zgw_monitor->cluster_traffic(),
           g_zgw_monitor->request_count(),
           g_zgw_monitor->failed_request(),
           g_zgw_monitor->avg_upload_part_time(),
           g_zgw_monitor->auth_failed_count());

  std::string result(buf);
  result.append(GenBucketInfo(force));
  result.append("], \"commands_info\": [");
  result.append(GenCommandsInfo());
  result.append("]}");

  return result;
}

void ZgwAdminHandles::Initialize() {
  command_.clear();
  params_.clear();
  result_.clear();
  http_ret_code_ = 200;
  store_ = reinterpret_cast<zgwstore::ZgwStore*>(worker_specific_data_);
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

std::string ZgwAdminHandles::GenBucketInfo(bool force) {
  char buf[1024];
  const char* format = "\
  {\
    \"name\": \"%s\",\
      \"traffic\": %lu,\
      \"volume\": %lu\
  },";
  static uint64_t _5minutes = 5 * 60 * 1e6;
  if (slash::NowMicros() - bk_update_time_ > _5minutes || force) {
    // Update Buckets meta info
    std::vector<zgwstore::User> all_users;
    store_->ListUsers(&all_users);
    http_ret_code_ = 200;
    all_buckets_.clear();
    for (auto& user : all_users) {
      std::vector<zgwstore::Bucket> user_bks;
      store_->ListBuckets(user.display_name, &user_bks);
      all_buckets_.insert(all_buckets_.begin(),
                          user_bks.begin(), user_bks.end());
    }
    bk_update_time_ = slash::NowMicros();
  }

  std::string bks_info;
  for (auto b : all_buckets_) {
    sprintf(buf, format,
            b.bucket_name.c_str(),
            g_zgw_monitor->bucket_traffic(b.bucket_name),
            b.volumn);
    bks_info.append(buf);
  }
  if (!bks_info.empty() && bks_info.back() == ',') {
    bks_info.resize(bks_info.size() - 1);
  }
  return bks_info;
}

std::string ZgwAdminHandles::GenCommandsInfo() {
  char buf[1024];
  const char* format = "\
  {\
    \"cmd\": \"%s\",\
      \"request_count\": \"%lu\",\
      \"4xx_error\": \"%lu\",\
      \"5xx_error\": \"%lu\"\
  },";
  std::vector<S3Commands> all_cmds {
    kListAllBuckets,
    kDeleteBucket,
    kListObjects,
    kGetBucketLocation,
    kHeadBucket,
    kListMultiPartUpload,
    kPutBucket,
    kDeleteObject,
    kDeleteMultiObjects,
    kGetObject,
    kHeadObject,
    kPostObject,
    kPutObject,
    kPutObjectCopy,
    kInitMultipartUpload,
    kUploadPart,
    kUploadPartCopy,
    kUploadPartCopyPartial,
    kCompleteMultiUpload,
    kAbortMultiUpload,
    kListParts,
    kUnImplement,
    kZgwTest,
  };

  std::string cmd_info;
  for (auto cmd : all_cmds) {
    sprintf(buf, format,
            S3CommandsToString(cmd),
            g_zgw_monitor->api_request_count(cmd),
            g_zgw_monitor->api_err4xx_count(cmd),
            g_zgw_monitor->api_err5xx_count(cmd));
    cmd_info.append(buf);
  }

  if (!cmd_info.empty() && cmd_info.back() == ',') {
    cmd_info.resize(cmd_info.size() - 1);
  }
  return cmd_info;
}

static const char* S3CommandsToString(S3Commands cmd_type) {
  switch(cmd_type) {
    case kListAllBuckets:
      return "ListAllBuckets: ";
    case kDeleteBucket:
      return "DeleteBucket: ";
    case kListObjects:
      return "ListObjects: ";
    case kGetBucketLocation:
      return "GetBucketLocation";
    case kHeadBucket:
      return "HeadBucket: ";
    case kListMultiPartUpload:
      return "ListMultiPartUpload: ";
    case kPutBucket:
      return "PutBucket: ";
    case kDeleteObject:
      return "DeleteObject: ";
    case kDeleteMultiObjects:
      return "DeleteMultiObjects: ";
    case kGetObject:
      return "GetObject: ";
    case kHeadObject:
      return "HeadObject: ";
    case kPostObject:
      return "PostObject: ";
    case kPutObject:
      return "PutObject: ";
    case kPutObjectCopy:
      return "PutObjectCopy: ";
    case kInitMultipartUpload:
      return "InitMultipartUpload: ";
    case kUploadPart:
      return "UploadPart: ";
    case kUploadPartCopy:
      return "UploadPartCopy: ";
    case kUploadPartCopyPartial:
      return "UploadPartCopyPartial: ";
    case kCompleteMultiUpload:
      return "CompleteMultiUpload: ";
    case kAbortMultiUpload:
      return "AbortMultiUpload: ";
    case kListParts:
      return "ListParts: ";
    case kZgwTest:
      return "ZgwTest: ";
    case kUnImplement:
    default:
      return "UnImplement: ";
  }
}
