#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/zgw_utils.h"

bool HeadBucketCmd::DoInitial() {
  request_id_ = md5(bucket_name_ +
                    std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(INFO) << request_id_ << " " <<
      "HeadBucket(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "HeadBucket(DoInitial) - " << bucket_name_;
  return true;
}

void HeadBucketCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->GetBucket(user_name_, bucket_name_, &bucket_);
    if (s.ok()) {
      // Just response 200 OK
    } else if (s.ToString().find("Bucket Doesn't Belong To This User") ||
               s.ToString().find("Bucket Not Found")) {
      http_ret_code_ = 404;
    } else if (s.IsIOError()){
      http_ret_code_ = 500;
      LOG(ERROR) << request_id_ << " " <<
        "HeadBucket(DoAndResponse) - GetBucket failed: " <<
        bucket_name_ << " " << s.ToString();
    }
  }

  g_zgw_monitor->AddApiRequest(kHeadBucket, http_ret_code_);
  resp->SetContentLength(0); // HEAD needn't response data
  resp->SetStatusCode(http_ret_code_);
}
