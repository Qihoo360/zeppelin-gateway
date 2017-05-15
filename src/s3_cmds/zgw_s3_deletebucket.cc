#include "src/s3_cmds/zgw_s3_bucket.h"

#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool DeleteBucketCmd::DoInitial() {
  http_response_xml_.clear();

  return TryAuth();
}

void DeleteBucketCmd::DoAndResponse(pink::HttpResponse* resp) {
  // Check if multipart uplaad exist
  std::vector<zgwstore::Bucket> all_buckets;
  Status s = store_->ListBuckets(user_name_, &all_buckets);
  if (!s.ok()) {
    http_ret_code_ = 500;
  }

  // Filter
  for (auto& b : all_buckets) {
    if (b.bucket_name.substr(0, 6) != "__TMPB") {
      // Skip normal bucket
      continue;
    }
    size_t bucket_name_pos = b.bucket_name.find("|");
    assert(bucket_name_pos != std::string::npos);
    std::string bucket_name = b.bucket_name.substr(6 + 32,
                                                   bucket_name_pos - 38);
    if (bucket_name == bucket_name_) {
      // Bucket not empty
      http_ret_code_ = 409;
      GenerateErrorXml(kBucketNotEmpty, bucket_name_);
      break;
    }
  }

  if (http_ret_code_ == 200) {
    s = store_->DeleteBucket(user_name_, bucket_name_);
    if (!s.ok()) {
      http_ret_code_ = 500;
    }
  }

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
