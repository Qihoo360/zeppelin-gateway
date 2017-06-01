#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "slash/include/slash_hash.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool ListObjectsCmd::DoInitial() {
  all_objects_name_.clear();
  http_response_xml_.clear();

  if (!TryAuth()) {
    DLOG(INFO) <<
      "ListObjects(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  std::string invalid_param;
  list_typeV2_ = query_params_.count("list-type");
  if (!SanitizeParams(&invalid_param)) {
    http_ret_code_ = 400;
    GenerateErrorXml(kInvalidArgument, invalid_param);
    return false;
  }

  request_id_ = md5(bucket_name_ +
                    delimiter_ +
                    prefix_ +
                    std::to_string(max_keys_) +
                    std::to_string(slash::NowMicros()));

  DLOG(INFO) << request_id_ << " " <<
    "ListObjects(DoInitial) - " << bucket_name_;
  return true;
}

void ListObjectsCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->ListObjectsName(user_name_, bucket_name_, &all_objects_name_);
    if (s.ok()) {
      GenerateRespXml();
    } else if (s.ToString().find("Bucket Doesn't Belong To This User") !=
               std::string::npos) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    } else {
      http_ret_code_ = 500;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
      LOG(ERROR) << request_id_ << " " <<
        "ListObjects(DoAndResponse) - ListObjects failed: " <<
        bucket_name_ << " " << s.ToString();
    }
  }

  g_zgw_monitor->AddApiRequest(kListObjects, http_ret_code_);
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
  // Select objects according to request params

  std::vector<zgwstore::Object> candidate_objects;
  std::vector<std::string> candidate_obj_names;
  std::set<std::string> commonprefixes;

  std::sort(all_objects_name_.begin(), all_objects_name_.end(),
            [](const std::string& a, const std::string& b) {
              return a < b;
            });

  for (auto& name : all_objects_name_) {
    if (!prefix_.empty() &&
        name.substr(0, prefix_.size()) != prefix_) {
      // Skip prefix
      continue;
    }
    if (!continuation_token_.empty() && list_typeV2_ &&
        name.substr(0, continuation_token_.size()) < continuation_token_) {
      // Skip to continuation_token
      continue;
    }
    if (!start_after_.empty() && list_typeV2_ &&
        name.substr(0, start_after_.size()) < start_after_) {
      // Skip start after v2
      continue;
    }
    if (!marker_.empty() && !list_typeV2_ &&
        name.substr(0, marker_.size()) <= marker_) {
      // Skip marker v1
      continue;
    }
    if (!delimiter_.empty()) {
      size_t pos = name.find_first_of(delimiter_, prefix_.size());
      if (pos != std::string::npos) {
        // Avoid duplicate
        commonprefixes.insert(name.substr(0, pos + 1));
        continue;
      }
    }

    candidate_obj_names.push_back(name);
  }

  int diff = candidate_obj_names.size() + commonprefixes.size() - max_keys_;
  bool is_trucated = diff > 0;

  std::string next_token;
  if (is_trucated) {
    size_t extra_num = diff;
    if (commonprefixes.size() >= extra_num) {
      for (auto it = commonprefixes.rbegin();
           it != commonprefixes.rend() && extra_num--;) {
        next_token = *(it++);
        commonprefixes.erase(next_token);
      }
    } else {
      // extra_num > commonprefixes.size()
      extra_num -= commonprefixes.size();
      commonprefixes.clear();
      if (candidate_obj_names.size() <= extra_num) {
        candidate_obj_names.clear();
      } else {
        while (extra_num--) {
          next_token = candidate_obj_names.back();
          candidate_obj_names.pop_back();
        }
      }
    }
  }
  // Is not trucated if max keys equal zero
  is_trucated = is_trucated && max_keys_ > 0;
  std::string next_marker;
  if (is_trucated &&
      !delimiter_.empty()) {
    if (!commonprefixes.empty()) {
      next_marker = *commonprefixes.rbegin();
    } else if (!candidate_obj_names.empty()) {
      next_marker = candidate_obj_names.back();
    }
  }

  if (!is_trucated) {
    next_token.clear();
  }
  size_t key_count = candidate_obj_names.size() + commonprefixes.size();

  Status s = store_->MGetObjects(user_name_, bucket_name_,
                         candidate_obj_names, &candidate_objects);
  if (s.IsIOError()) {
    http_ret_code_ = 500;
    LOG(ERROR) << request_id_ << " " <<
      "ListObjects(DoAndResponse) - MGetObjects failed: " <<
      bucket_name_ << " " << s.ToString();
    return;
  }
  if (candidate_obj_names.size() != candidate_objects.size()) {
    LOG(ERROR) << request_id_ << " " <<
      "ListObjects(DoAndResponse) - MGetObjects some object doestn't exist: " <<
      bucket_name_ << " " << s.ToString();
  }

  // Build response XML
  S3XmlDoc doc("ListBucketResult");
  
  doc.AppendToRoot(doc.AllocateNode("Name", bucket_name_));
  doc.AppendToRoot(doc.AllocateNode("Prefix", prefix_));
  doc.AppendToRoot(doc.AllocateNode("MaxKeys", std::to_string(max_keys_)));
  if (!delimiter_.empty()) {
    doc.AppendToRoot(doc.AllocateNode("Delimiter", delimiter_));
  }
  if (list_typeV2_) {
    if (!continuation_token_.empty()) {
      doc.AppendToRoot(doc.AllocateNode("ContinuationToken", continuation_token_));
    }
    if (!next_token.empty()) {
      doc.AppendToRoot(doc.AllocateNode("NextContinuationToken", next_token));
    }
    if (!start_after_.empty()) {
      doc.AppendToRoot(doc.AllocateNode("StartAfter", start_after_));
    }
    doc.AppendToRoot(doc.AllocateNode("KeyCount", std::to_string(key_count)));
  } else {
    doc.AppendToRoot(doc.AllocateNode("Marker", marker_));
    if (is_trucated) {
      doc.AppendToRoot(doc.AllocateNode("NextMarker", next_marker));
    }
  }
  doc.AppendToRoot(doc.AllocateNode("IsTruncated",
                                    is_trucated ? "true" : "false"));

  for (auto& o : candidate_objects) {
    S3XmlNode* contents = doc.AllocateNode("Contents");
    contents->AppendNode(doc.AllocateNode("Key", o.object_name));
    contents->AppendNode(doc.AllocateNode("LastModified",
                                          iso8601_time(o.last_modified)));
    contents->AppendNode(doc.AllocateNode("ETag", "\"" + o.etag + "\""));
    contents->AppendNode(doc.AllocateNode("Size", std::to_string(o.size)));
    contents->AppendNode(doc.AllocateNode("StorageClass", "STANDARD"));
    if (!list_typeV2_ || (list_typeV2_ && fetch_owner_)) {
      S3XmlNode* owner = doc.AllocateNode("Owner");
      owner->AppendNode(doc.AllocateNode("ID", slash::sha256(o.owner)));
      owner->AppendNode(doc.AllocateNode("DisplayName", o.owner));
      contents->AppendNode(owner);
    }
    doc.AppendToRoot(contents);
  }
  for (auto& p : commonprefixes) {
    S3XmlNode* comprefixes = doc.AllocateNode("CommonPrefixes");
    comprefixes->AppendNode(doc.AllocateNode("Prefix", p));
    doc.AppendToRoot(comprefixes);
  }

  doc.ToString(&http_response_xml_);
}

bool ListObjectsCmd::SanitizeParams(std::string* invalid_param) {
  delimiter_.clear();
  max_keys_ = 1000;
  prefix_.clear();

  marker_.clear();

  fetch_owner_ = false;
  continuation_token_.clear();
  start_after_.clear();

  if (query_params_.count("delimiter")) {
    delimiter_ = query_params_.at("delimiter");
    if (delimiter_.size() != 1) {
      invalid_param->assign("delimiter");
      return false;
    }
  }
  if (query_params_.count("prefix")) {
    prefix_ = query_params_.at("prefix");
  }
  if (query_params_.count("max-keys")) {
    std::string max_keys = query_params_.at("max-keys");
    max_keys_ = std::atoi(max_keys.c_str());
    if (max_keys_ < 0 ||
        max_keys_ > 1000 ||
        max_keys.empty() ||
        !isdigit(max_keys.at(0))) {
      invalid_param->assign("max-keys");
      return false;
    }
  }

  if (list_typeV2_) {
    if (query_params_.at("list-type") != "2") {
      invalid_param->assign("list-type");
      return false;
    }
    if (query_params_.count("fetch-owner")) {
      std::string fetch_owner = query_params_.at("fetch-owner");
      if (fetch_owner == "true") {
        fetch_owner_ = true;
      } else if (fetch_owner == "false") {
        fetch_owner_ = false;
      } else {
        invalid_param->assign("fetch-owner");
        return false;
      }
    }
    if (query_params_.count("continuation-token")) {
      continuation_token_ = query_params_.at("continuation-token");
    }
    if (query_params_.count("start-after")) {
      start_after_ = query_params_.at("start-after");
    }
  } else {
    if (query_params_.count("marker")) {
      marker_ = query_params_.at("marker");
    }
  }

  return true;
}
