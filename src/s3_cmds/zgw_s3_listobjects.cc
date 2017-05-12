#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/slash_hash.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool ListObjectsCmd::DoInitial() {
  all_objects_.clear();
  http_response_xml_.clear();

  return TryAuth();
}

void ListObjectsCmd::DoAndResponse(pink::HttpResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->ListObjects(user_name_, bucket_name_, &all_objects_);
    if (s.ok()) {
      // Build response XML using all_objects_
      GenerateRespXml();
    } else if (s.IsNotFound()) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    } else {
      http_ret_code_ = 500;
    }
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int ListObjectsCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}

void ListObjectsCmd::GenerateRespXml() {
  S3XmlDoc doc("ListBucketResult");
  
  S3XmlNode* user = doc.AllocateNode("Owner");
  user->AppendNode(doc.AllocateNode("ID", slash::sha256(user_name_)));
  user->AppendNode(doc.AllocateNode("DisplayName", user_name_));

  doc.AppendToRoot(user);

  S3XmlNode* buckets = doc.AllocateNode("Objects");
  for (auto& o : all_objects_) {
    S3XmlNode* bucket = doc.AllocateNode("Bucket");
    bucket->AppendNode(doc.AllocateNode("Name", o.object_name));
    bucket->AppendNode(doc.AllocateNode("CreationDate",
                                        iso8601_time(o.last_modified)));
    buckets->AppendNode(bucket);
  }
  doc.AppendToRoot(buckets);

  doc.ToString(&http_response_xml_);
}
