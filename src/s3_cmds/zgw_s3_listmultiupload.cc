#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/slash_hash.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool ListMultiPartUploadCmd::DoInitial(pink::HTTPResponse* resp) {
  all_virtual_bks_.clear();
  http_response_xml_.clear();

  if (!TryAuth()) {
    return false;
  }

  std::string invalid_param;
  if (!SanitizeParams(&invalid_param)) {
    http_ret_code_ = 400;
    GenerateErrorXml(kInvalidArgument, invalid_param);
    return false;
  }

  return true;
}

bool ListMultiPartUploadCmd::SanitizeParams(std::string* invalid_param) {
  delimiter_.clear();
  prefix_.clear();
  max_uploads_ = 1000;
  key_marker_.clear();
  upload_id_marker_.clear();

  if (query_params_.count("delimiter")) {
    if (query_params_.at("delimiter").size() != 1) {
      invalid_param->assign("delimiter");
      return false;
    }
    delimiter_ = query_params_.at("delimiter");
  }
  if (query_params_.count("prefix")) {
    prefix_ = query_params_.at("prefix");
  }
  if (query_params_.count("max-uploads")) {
    std::string max_uploads = query_params_.at("max-uploads");
    max_uploads_ = std::atoi(max_uploads.c_str());
    if (max_uploads_ < 0 ||
        max_uploads_ > 1000 ||
        max_uploads.empty() ||
        !isdigit(max_uploads.at(0))) {
      invalid_param->assign("max-uploads");
      return false;
    }
  }
  if (query_params_.count("key-marker")) {
    key_marker_ = query_params_.at("key-marker");
  }
  if (query_params_.count("upload-id-marker")) {
    key_marker_ = query_params_.at("upload-id-marker");
  }

  return true;
}

void ListMultiPartUploadCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    std::vector<zgwstore::Bucket> all_buckets;
    Status s = store_->ListBuckets(user_name_, &all_buckets);
    if (!s.ok()) {
      http_ret_code_ = 500;
    }

    // Sorting
    for (auto& b : all_buckets) {
      if (b.bucket_name.substr(0, 6) != "__TMPB") {
        // Skip normal bucket
        continue;
      }
      size_t bucket_name_pos = b.bucket_name.find("|");
      assert(bucket_name_pos != std::string::npos);
      std::string bucket_name = b.bucket_name.substr(6 + 32,
                                                     bucket_name_pos - 38);
      if (bucket_name != bucket_name_) {
        // Skip other bucket
        continue;
      }

      all_virtual_bks_.insert(b);
    }
  }

  if (http_ret_code_ == 200) {
    // Build response XML using all_virtual_bks_
    GenerateRespXml();
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int ListMultiPartUploadCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}

void ListMultiPartUploadCmd::GenerateRespXml() {
  // object_name = bucketname.substr(prefix size + upload id size);

  std::vector<zgwstore::Bucket> candidate_buckets;
  std::set<std::string> commonprefixes;

  for (auto& b : all_virtual_bks_) {
    size_t pos = b.bucket_name.find("|");
    assert(pos != std::string::npos);
    std::string name = b.bucket_name.substr(pos + 1);
    std::string upload_id = b.bucket_name.substr(6, 32);

    if (!prefix_.empty() &&
        name.substr(0, prefix_.size()) != prefix_) {
      // Skip prefix
      continue;
    }
    if (!key_marker_.empty()) {
      // If upload-id-marker is specified
      if (!upload_id_marker_.empty() &&
          name <= key_marker_ &&
          upload_id <= upload_id_marker_) {
        continue;
      }
      // If upload-id-marker is not specified
      if (upload_id_marker_.empty() &&
          name <= key_marker_) {
        continue;
      }
    } else {
      // Ignore upload-id-marker if key-marker is not specifies
    }
    if (!delimiter_.empty()) {
      size_t pos = name.find_first_of(delimiter_, prefix_.size());
      if (pos != std::string::npos) {
        commonprefixes.insert(name.substr(0, std::min(name.size(), pos + 1)));
        continue;
      }
    }

    candidate_buckets.push_back(b);
  }

  int diff = commonprefixes.size() + candidate_buckets.size()
    - static_cast<size_t>(max_uploads_);
  bool is_trucated = diff > 0;
  if (is_trucated) {
    size_t extra_num = diff;
    if (commonprefixes.size() >= extra_num) {
      for (auto it = commonprefixes.rbegin();
           it != commonprefixes.rend() && extra_num--;) {
        commonprefixes.erase(*(it++));
      }
    } else {
      extra_num -= commonprefixes.size();
      commonprefixes.clear();
      if (candidate_buckets.size() <= extra_num) {
        candidate_buckets.clear();
      } else {
        while (extra_num--)
          candidate_buckets.pop_back();
      }
    }
  }

  is_trucated = is_trucated && max_uploads_ > 0;
  std::string next_key_marker, next_upload_id;
  if (is_trucated) {
    if (!commonprefixes.empty()) {
      next_key_marker = *commonprefixes.rbegin();
    } else if (!candidate_buckets.empty()) {
      auto& vir_b_name = candidate_buckets.back().bucket_name;
      size_t pos = vir_b_name.find("|");
      next_key_marker = vir_b_name.substr(pos + 1);
      next_upload_id = vir_b_name.substr(6, 32);
    }
  }

  S3XmlDoc doc("ListMultipartUploadsResult");
  
  doc.AppendToRoot(doc.AllocateNode("Bucket", bucket_name_));
  doc.AppendToRoot(doc.AllocateNode("KeyMarker", key_marker_));
  doc.AppendToRoot(doc.AllocateNode("UploadIdMarker", upload_id_marker_));
  if (!next_key_marker.empty()) {
    doc.AppendToRoot(doc.AllocateNode("NextKeyMarker", next_key_marker));
  }
  if (!next_upload_id.empty()) {
    doc.AppendToRoot(doc.AllocateNode("NextUploadIdMarker", next_upload_id));
  }
  doc.AppendToRoot(doc.AllocateNode("MaxUploads", std::to_string(max_uploads_)));
  doc.AppendToRoot(doc.AllocateNode("IsTruncated", is_trucated ? "true" : "false"));

  for (auto& b : candidate_buckets) {
    auto& vir_b_name = b.bucket_name;
    size_t pos = vir_b_name.find("|");
    std::string key = vir_b_name.substr(pos + 1);
    std::string upload_id = vir_b_name.substr(6, 32);

    S3XmlNode* upload_node = doc.AllocateNode("Upload");
    doc.AppendToRoot(upload_node);
    upload_node->AppendNode(doc.AllocateNode("Key", key));
    upload_node->AppendNode(doc.AllocateNode("UploadId", upload_id));

    S3XmlNode* owner_node = doc.AllocateNode("Onwer");
    owner_node->AppendNode("ID", slash::sha256(b.owner));
    owner_node->AppendNode("DisplayName", b.owner);
    upload_node->AppendNode(owner_node);
    S3XmlNode* initialtor_node = doc.AllocateNode("Initiator");
    initialtor_node->AppendNode("ID", slash::sha256(b.owner));
    initialtor_node->AppendNode("DisplayName", b.owner);
    upload_node->AppendNode(initialtor_node);

    upload_node->AppendNode("StorageClass", "STANDARD");
    upload_node->AppendNode("Initiated", iso8601_time(b.create_time));
  }

  doc.ToString(&http_response_xml_);
}
