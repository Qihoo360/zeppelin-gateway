#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "slash/include/slash_hash.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool ListAllBucketsCmd::DoInitial() {
  all_buckets_.clear();
  http_response_xml_.clear();
  request_id_ = md5(std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(ERROR) << request_id_ << " " <<
      "ListAllBuckets(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "ListAllBuckets(DoInitial) - " << user_name_;
  return true;
}

void ListAllBucketsCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->ListBuckets(user_name_, &all_buckets_);
    if (!s.ok()) {
      http_ret_code_ = 500;
      LOG(ERROR) << request_id_ << " " <<
        "ListAllBuckets(DoAndResponse) - ListBuckets failed: " <<
        user_name_ << " " << s.ToString();
    }

    // Build response XML using all_buckets_
    GenerateRespXml();
  }

  g_zgw_monitor->AddApiRequest(kListAllBuckets, http_ret_code_);
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int ListAllBucketsCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}

void ListAllBucketsCmd::GenerateRespXml() {
  S3XmlDoc doc("ListAllMyBucketsResult");
  
  S3XmlNode* user = doc.AllocateNode("Owner");
  user->AppendNode(doc.AllocateNode("ID", slash::sha256(user_name_)));
  user->AppendNode(doc.AllocateNode("DisplayName", user_name_));

  doc.AppendToRoot(user);

  S3XmlNode* buckets = doc.AllocateNode("Buckets");
  for (auto& b : all_buckets_) {
    if (b.bucket_name.substr(0, 6) == "__TMPB") {
      continue;
    }
    S3XmlNode* bucket = doc.AllocateNode("Bucket");
    bucket->AppendNode(doc.AllocateNode("Name", b.bucket_name));
    bucket->AppendNode(doc.AllocateNode("CreationDate",
                                        iso8601_time(b.create_time)));
    buckets->AppendNode(bucket);
  }
  doc.AppendToRoot(buckets);

  doc.ToString(&http_response_xml_);
}
