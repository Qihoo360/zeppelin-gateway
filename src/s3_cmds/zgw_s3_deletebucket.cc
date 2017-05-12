#include "src/s3_cmds/zgw_s3_bucket.h"

#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_util.h"

bool DeleteBucketCmd::DoInitial() {
  http_response_xml_.clear();

  return TryAuth();
}

void DeleteBucketCmd::DoReceiveBody(const char* data, size_t data_size) {
}

void DeleteBucketCmd::DoAndResponse(pink::HttpResponse* resp) {
  if (http_ret_code_ == 200) {
    // Status s = store_->DelBucket(user_name_, bucket_name);
    Status s;
    if (!s.ok()) {
      // Not Empty
      // IO Error
      http_ret_code_ = 500;
    }

    // Build response XML using all_buckets_
    // GenerateRespXml();
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int DeleteBucketCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}
