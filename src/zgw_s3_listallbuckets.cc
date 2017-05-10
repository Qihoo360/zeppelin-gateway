#include "src/zgw_s3_bucket.h"

#include "slash/include/slash_hash.h"
#include "src/zgw_xml.h"

bool ListAllBucketsCmd::DoInitial() {
  all_buckets_.clear();
  http_response_xml_.clear();

  switch(s3_auth_.TryAuth()) {
    case kMaybeAuthV2:
      http_ret_code_ = 400;
      GenerateErrorXml(kInvalidRequest, "Please use AWS4-HMAC-SHA256.");
      return false;
      break;
    case kAccessKeyInvalid:
      http_ret_code_ = 403;
      GenerateErrorXml(kInvalidAccessKeyId);
      return false;
      break;
    case kSignatureNotMatch:
      http_ret_code_ = 403;
      GenerateErrorXml(kSignatureDoesNotMatch);
      return false;
      break;
    case kAuthSuccess:
    default:
      break;
  }

  user_name_.assign(s3_auth_.user_name());

  return true;
}

void ListAllBucketsCmd::DoAndResponse(pink::HttpResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->ListBuckets(user_name_, &all_buckets_);
    if (!s.ok()) {
      http_ret_code_ = 500;
    }

    // Build response XML using all_buckets_
    GenerateRespXml();
  }

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

  doc.ToString(&http_response_xml_);
}
