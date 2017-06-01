#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool GetBucketLocationCmd::DoInitial() {
  http_response_xml_.clear();
  request_id_ = md5(bucket_name_ +
                    std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(INFO) << request_id_ << " " <<
      "GetBucketLocation(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "GetBucketLocation(DoInitial) - " << bucket_name_;
  return true;
}

void GetBucketLocationCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->GetBucket(user_name_, bucket_name_, &bucket_);
    if (s.ok()) {
      GenerateRespXml();
    } else if (s.ToString().find("Bucket Doesn't Belong To This User") ||
               s.ToString().find("Bucket Not Found")) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    } else {
      http_ret_code_ = 500;
      LOG(ERROR) << request_id_ << " " <<
        "GetBucketLocation(DoAndResponse) - GetBucket failed: " <<
        bucket_name_ << " " << s.ToString();
    }
  }

  g_zgw_monitor->AddApiRequest(kGetBucketLocation, http_ret_code_);
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int GetBucketLocationCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}

void GetBucketLocationCmd::GenerateRespXml() {
  S3XmlDoc doc("LocationConstraint", bucket_.location);
  doc.ToString(&http_response_xml_);
}
