#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"

bool DeleteObjectCmd::DoInitial() {
  http_response_xml_.clear();
  DLOG(INFO) << "DeleteObject(DoInitial) - " << bucket_name_ << "/" <<
    object_name_;

  return TryAuth();
}

void DeleteObjectCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    zgwstore::Bucket dummy_bk;
    Status s = store_->GetBucket(user_name_, bucket_name_, &dummy_bk);
    if (!s.ok()) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    }
  }

  if (http_ret_code_ == 200) {
    Status s = store_->DeleteObject(user_name_, bucket_name_, object_name_);
    if (s.ok()) {
      http_ret_code_ = 204;
    } else if (s.ToString().find("Bucket Doesn't Belong To This User") !=
               std::string::npos) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    }
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int DeleteObjectCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
