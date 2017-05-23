#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"

bool HeadObjectCmd::DoInitial() {
  request_id_ = md5(bucket_name_ +
                    object_name_ +
                    std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(ERROR) << request_id_ << " " <<
      "HeadObject(DoInitial) - Auth failed: " << client_ip_port_;
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "HeadObject(DoInitial) - " << bucket_name_ << "/" << object_name_;
  return true;
}

void HeadObjectCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->GetObject(user_name_, bucket_name_, object_name_,
                                 &object_);
    if (s.ok()) {
      http_ret_code_ = 200;
      resp->SetHeaders("ETag", "\"" + object_.etag + "\"");
      resp->SetHeaders("Last-Modified", http_nowtime(object_.last_modified));
    } else {
      if (s.ToString().find("Bucket Doesn't Belong To This User") !=
          std::string::npos) {
        http_ret_code_ = 404;
      } else if (s.ToString().find("Object Not Found") != std::string::npos) {
        http_ret_code_ = 404;
      } else if (s.IsIOError()) {
        LOG(ERROR) << request_id_ << " " <<
          "HeadObject(DoAndResponse) - GetObject failed: " <<
          bucket_name_ << "/" << object_name_ << " " << s.ToString();
        http_ret_code_ = 500;
      }
    }
  }

  if (http_ret_code_ == 200) {
    resp->SetHeaders("Content-Length", object_.size);
  }
  resp->SetContentLength(0);
  resp->SetStatusCode(http_ret_code_);
}
