#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgw_utils.h"

bool AbortMultiUploadCmd::DoInitial() {
  http_response_xml_.clear();
  upload_id_ = query_params_.at("uploadId");
  request_id_ = md5(bucket_name_ +
                    object_name_ +
                    upload_id_ + 
                    std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(ERROR) << request_id_ << " " <<
      "AbortMultiUpload(DoInitial) - Auth failed: " << client_ip_port_;
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "AbortMultiUpload(DoInitial) - " << bucket_name_ << "/" << object_name_ <<
    ", uploadId: " << upload_id_;
  return true;
}

void AbortMultiUploadCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s;
    std::string virtual_bucket = "__TMPB" + upload_id_ +
      bucket_name_ + "|" + object_name_;

    s = store_->Lock();
    if (s.ok()) {
      zgwstore::Bucket dummy_bk;
      s = store_->GetBucket(user_name_, virtual_bucket, &dummy_bk);
      if (!s.ok()) {
        if (s.ToString().find("Bucket Doesn't Belong To This User") ||
            s.ToString().find("Bucket Not Found")) {
          http_ret_code_ = 404;
          GenerateErrorXml(kNoSuchUpload, upload_id_);
          resp->SetContentLength(http_response_xml_.size());
        } else {
          LOG(ERROR) << request_id_ << " " <<
            "AbortMultiUpload(DoAndResponse) - GetVirtBucket failed: " <<
            virtual_bucket << " " << s.ToString();
          http_ret_code_ = 500;
        }
      } else {
        // Delete objects
        std::vector<zgwstore::Object> all_parts;
        s = store_->ListObjects(user_name_, virtual_bucket, &all_parts);
        if (!s.ok()) {
          LOG(ERROR) << request_id_ << " " <<
            "AbortMultiUpload(DoAndResponse) - ListVirtObjects failed: " <<
            virtual_bucket << " " << s.ToString();
          http_ret_code_ = 500;
        } else {
          for (auto& p : all_parts) {
            s = store_->DeleteObject(user_name_, virtual_bucket, p.object_name, false);
            if (s.IsIOError()) {
              LOG(ERROR) << request_id_ << " " <<
                "AbortMultiUpload(DoAndResponse) - DeleteVirtObject failed: " <<
                virtual_bucket << "/" << p.object_name << " " << s.ToString();
              http_ret_code_ = 500;
            }
          }
          if (http_ret_code_ == 200) {
            s = store_->DeleteBucket(user_name_, virtual_bucket, false);
            if (s.IsIOError()) {
              LOG(ERROR) << request_id_ << " " <<
                "AbortMultiUpload(DoAndResponse) - DeleteVirtBucketfailed: " <<
                virtual_bucket << " " << s.ToString();
              http_ret_code_ = 500;
            }
          }

          // Success delete
          http_ret_code_ = 204;
          LOG(INFO) << request_id_ << " " <<
            "AbortMultiUpload(DoAndResponse) - Success: " << virtual_bucket;
        }
      }

      s = store_->UnLock();
    }
    if (!s.ok()) {
      LOG(ERROR) << request_id_ << " " <<
        "AbortMultiUpload(DoAndResponse) - Lock or Unlock failed: " <<
        s.ToString();
      http_ret_code_ = 500;
    }
    DLOG(INFO) << request_id_ << " " <<
      "AbortMultiUpload(DoAndResponse) - Unlock success: " << virtual_bucket;
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int AbortMultiUploadCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}
