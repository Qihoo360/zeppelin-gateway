#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool DeleteBucketCmd::DoInitial() {
  http_response_xml_.clear();
  request_id_ = md5(bucket_name_ +
                    std::to_string(slash::NowMicros()));

  if (!TryAuth()) {
    DLOG(ERROR) << request_id_ << " " <<
      "DeleteBucket(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "DeleteBucket(DoInitial) - " << bucket_name_;

  return TryAuth();
}

void DeleteBucketCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->Lock();
    if (s.ok()) {
      // Check if multipart uplaad exist
      std::vector<zgwstore::Bucket> all_buckets;
      s = store_->ListBuckets(user_name_, &all_buckets);
      if (!s.ok()) {
        http_ret_code_ = 500;
        LOG(ERROR) << request_id_ << " " <<
          "DeleteBucket(DoAndResponse) - ListBuckets failed " << s.ToString();
      } else {

        // Filter
        for (auto& b : all_buckets) {
          if (b.bucket_name.substr(0, 6) == "__TMPB") {
            std::string virtual_bucket = b.bucket_name;
            size_t bucket_name_pos = virtual_bucket.find("|");
            assert(bucket_name_pos != std::string::npos);
            std::string bucket_name =
              virtual_bucket.substr(6 + 32, bucket_name_pos - 38);
            if (bucket_name == bucket_name_) {
              // Delete all multipart upload
              std::vector<zgwstore::Object> all_parts;
              s = store_->ListObjects(user_name_, virtual_bucket, &all_parts);
              if (!s.ok()) {
                http_ret_code_ = 500;
                LOG(ERROR) << request_id_ << " " <<
                  "DeleteBucket(DoAndResponse) - ListVirtObjects failed: " <<
                  virtual_bucket << " " << s.ToString();
              } else {
                for (auto& p : all_parts) {
                  s = store_->DeleteObject(user_name_, virtual_bucket, p.object_name, false);
                  if (s.IsIOError()) {
                    http_ret_code_ = 500;
                    LOG(ERROR) << request_id_ << " " <<
                      "DeleteBucket(DoAndResponse) - DeleteVirtObject failed: " <<
                      virtual_bucket << "/" << p.object_name << " " << s.ToString();
                  }
                }
                if (http_ret_code_ == 200) {
                  s = store_->DeleteBucket(user_name_, virtual_bucket, false);
                  if (s.IsIOError()) {
                    http_ret_code_ = 500;
                    LOG(ERROR) << request_id_ << " " <<
                      "DeleteBucket(DoAndResponse) - DeleteVirtBucket failed: " <<
                      virtual_bucket << " " << s.ToString();
                  }
                }
              }
            }
          }
        }
      }

      if (http_ret_code_ == 200) {
        s = store_->DeleteBucket(user_name_, bucket_name_, false);
        if (!s.ok()) {
          if (s.ToString().find("Bucket Doesnt Exist") != std::string::npos) {
            http_ret_code_ = 404;
            GenerateErrorXml(kNoSuchBucket, bucket_name_);
          } else if (s.ToString().find("Bucket Vol IS NOT 0") != std::string::npos ||
                     s.ToString().find("Bucket Non Empty") != std::string::npos) {
            http_ret_code_ = 409;
            GenerateErrorXml(kBucketNotEmpty, bucket_name_);
          } else {
            http_ret_code_ = 500;
            LOG(ERROR) << request_id_ << " " <<
              "DeleteBucket(DoAndResponse) - DeleteBucket failed: " <<
              bucket_name_ << " " << s.ToString();
          }
        } else {
          http_ret_code_ = 204;
        }
      }
      s = store_->UnLock();
    }
    if (!s.ok()) {
      http_ret_code_ = 500;
      LOG(ERROR) << request_id_ << " " <<
        "DeleteBucket(DoAndResponse) - Lock or UnLock failed: " <<
        s.ToString();
    }
  }

  g_zgw_monitor->AddApiRequest(kDeleteBucket, http_ret_code_);
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int DeleteBucketCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}
