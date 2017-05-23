#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_xml.h"

bool PutBucketCmd::DoInitial() {
  http_request_xml_.clear();
  http_response_xml_.clear(); 

  request_id_ = md5(bucket_name_ +
                    std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(ERROR) << request_id_ << " " <<
      "PutBucket(DoInitial) - Auth failed: " << client_ip_port_;
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "PutBucket(DoInitial) - " << bucket_name_;
  return true;
}

void PutBucketCmd::DoReceiveBody(const char* data, size_t data_size) {
  http_request_xml_.append(data, data_size);
}

void PutBucketCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    S3XmlDoc doc;
    S3XmlNode root, loc;
    if (!http_request_xml_.empty() &&
        (!doc.ParseFromString(http_request_xml_) ||
         !doc.FindFirstNode("CreateBucketConfiguration", &root) ||
         !root.FindFirstNode("LocationConstraint", &loc))) {
      http_ret_code_ = 400;
      GenerateErrorXml(kMalformedXML);
    } else {
      // Initial new_bucket 
      new_bucket_.bucket_name = bucket_name_;
      new_bucket_.create_time = slash::NowMicros();
      new_bucket_.owner = user_name_;
      new_bucket_.acl = "FULL_CONTROL"; // TODO (gaodq) acl
      new_bucket_.location = loc.value().empty() ? "CN" : loc.value();
      new_bucket_.volumn = 0;
      new_bucket_.uploading_volumn = 0;

      Status s = store_->AddBucket(new_bucket_);
      if (s.ok()) {
        http_ret_code_ = 200;
      } else {
        std::string error_info = s.ToString();
        if (error_info.find("Bucket Already Exist [GLOBAL]") != std::string::npos) {
          http_ret_code_ = 409;
          GenerateErrorXml(kBucketAlreadyExists);
        } else if (error_info.find("Bucket Already Exist") != std::string::npos) {
          http_ret_code_ = 409;
          GenerateErrorXml(kBucketAlreadyOwnedByYou);
        } else {
          LOG(ERROR) << request_id_ << " " <<
            "PutBucket(DoAndResponse) - AddBucket failed: " <<
            bucket_name_ << " " << s.ToString();
          http_ret_code_ = 500;
        }
      }
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
