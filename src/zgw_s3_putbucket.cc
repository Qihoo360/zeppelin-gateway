#include "src/zgw_s3_bucket.h"

#include "slash/include/env.h"

bool PutBucketCmd::DoInitial() {
  // Initial new_bucket 
  new_bucket_.bucket_name = bucket_name_;
  new_bucket_.create_time = slash::NowMicros();
  new_bucket_.owner = "gaodq"; // TODO (gaodq)
  new_bucket_.acl = "FULL_CONTROL"; // TODO (gaodq)
  new_bucket_.location = "sh"; // TODO (gaodq)
  new_bucket_.volumn = 0;
  new_bucket_.uploading_volumn = 0;

  // Needn't reply right now
  return false;
}

void PutBucketCmd::DoReceiveBody(const char* data, size_t data_size) {

}

void PutBucketCmd::DoAndResponse(pink::HttpResponse* resp) {
  Status s = store_->AddBucket(new_bucket_);
  if (s.ok()) {
    http_ret_code_ = 200;
  } else {
    std::string error_info = s.ToString();
    if (error_info.find("Bucket Already Exist [GLOBAL]") != std::string::npos) {
      http_ret_code_ = 409;
    } else if (error_info.find("Bucket Already Exist") != std::string::npos) {
      http_ret_code_ = 409;
    } else {
      http_ret_code_ = 500;
    }
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int PutBucketCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
