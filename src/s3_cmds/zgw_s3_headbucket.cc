#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/zgw_utils.h"

bool HeadBucketCmd::DoInitial(pink::HTTPResponse* resp) {
  DLOG(INFO) << "HeadBucket(DoInitial) - " << bucket_name_;

  return TryAuth();
}

void HeadBucketCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->GetBucket(user_name_, bucket_name_, &bucket_);
    if (s.ok()) {
      // Just response 200 OK
    } else if (s.ToString().find("Bucket Doesn't Belong To This User") ||
               s.ToString().find("Bucket Not Found")) {
      http_ret_code_ = 404;
    } else {
      http_ret_code_ = 500;
    }
  }

  resp->SetContentLength(0); // HEAD needn't response data
  resp->SetStatusCode(http_ret_code_);
}
