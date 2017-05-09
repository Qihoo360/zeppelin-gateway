#include "src/zgw_s3_bucket.h"

#include "slash/include/slash_hash.h"
#include "src/zgw_xml.h"

bool ListAllBucketsCmd::DoInitial() {
  all_buckets_.clear();
  http_response_xml_.clear();

  // Needn't reply right now
  return false;
}

void ListAllBucketsCmd::DoAndResponse(pink::HttpResponse* resp) {
  Status s = store_->ListBuckets("gaodq", &all_buckets_);
  if (s.ok()) {
    http_ret_code_ = 200;
  } else if (s.IsNotFound()) {
    http_ret_code_ = 404;
    // TODO(gaodq) Set xml response
  } else {
    http_ret_code_ = 500;
  }

  // Build response XML using all_buckets_
  GenerateRespXml();

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
  user->AppendNode(doc.AllocateNode("ID", slash::sha256("gaodq")));
  user->AppendNode(doc.AllocateNode("DisplayName", "gaodq"));

  doc.AppendToRoot(user);

  S3XmlNode* buckets = doc.AllocateNode("Buckets");
  for (auto& b : all_buckets_) {
    S3XmlNode* bucket = doc.AllocateNode("Bucket");
    bucket->AppendNode(doc.AllocateNode("Name", b.bucket_name));
    bucket->AppendNode(doc.AllocateNode("CreationDate", b.bucket_name));
    buckets->AppendNode(bucket);
  }
  doc.AppendToRoot(buckets);

  http_response_xml_.assign(doc.ToString());
}
