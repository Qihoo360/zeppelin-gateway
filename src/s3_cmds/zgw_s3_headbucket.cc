#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/zgw_utils.h"

bool HeadBucketCmd::DoInitial() {

  return TryAuth();
}

void HeadBucketCmd::DoAndResponse(pink::HttpResponse* resp) {
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

int HeadBucketCmd::DoResponseBody(char* buf, size_t max_size) {
  return -2;
}
