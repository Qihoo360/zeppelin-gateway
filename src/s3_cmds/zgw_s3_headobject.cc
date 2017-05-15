#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"

bool HeadObjectCmd::DoInitial() {
  http_response_xml_.clear();

  return TryAuth();
}

void HeadObjectCmd::DoAndResponse(pink::HttpResponse* resp) {
  Status s = store_->GetObject(user_name_, bucket_name_, object_name_,
                               &object_);
  if (s.ok()) {
    http_ret_code_ = 200;
    resp->SetContentLength(object_.size);
    resp->SetHeaders("ETag", object_.etag);
    resp->SetHeaders("Last-Modified", http_nowtime(object_.last_modified));
  } else {
    if (s.ToString().find("Bucket Doesn't Belong To This User") !=
        std::string::npos) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    } else if (s.ToString().find("Object Not Found") != std::string::npos) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchKey, object_name_);
    }
    resp->SetContentLength(http_response_xml_.size());
  }

  resp->SetHeaders("Date", http_nowtime(slash::NowMicros()));
  resp->SetStatusCode(http_ret_code_);
}

int HeadObjectCmd::DoResponseBody(char* buf, size_t max_size) {
  if (http_ret_code_ == 200) {
    return -2; // Doesn't need response data
  }
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
