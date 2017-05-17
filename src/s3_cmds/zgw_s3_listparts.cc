#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/slash_hash.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool ListPartsCmd::DoInitial() {
  all_candicate_parts_.clear();
  http_response_xml_.clear();
  upload_id_ = query_params_.at("uploadId");
  max_parts_ = 1000;
  part_num_marker_ = "0";

  if (!TryAuth()) {
    return false;
  }

  if (query_params_.count("max-parts")) {
    std::string max_parts = query_params_.at("max-parts");
    max_parts_ = std::atoi(max_parts.c_str());
    if (max_parts_ < 0 ||
        max_parts_ > 1000 ||
        max_parts.empty() ||
        !isdigit(max_parts.at(0))) {
      http_ret_code_ = 400;
      GenerateErrorXml(kInvalidArgument, "max-parts");
      return false;
    }
  }
  if (query_params_.count("part-number-marker")) {
    part_num_marker_.assign(query_params_.at("part-number-marker"));
  }
  return true;
}

void ListPartsCmd::DoAndResponse(pink::HttpResponse* resp) {
  if (http_ret_code_ == 200) {
    std::vector<zgwstore::Object> all_parts;
    std::string virtual_bucket = "__TMPB" + upload_id_ +
      bucket_name_ + "|" + object_name_;
    Status s = store_->ListObjects(user_name_, virtual_bucket, &all_parts);
    if (s.ok()) {
      // Sorting
      for (auto& p : all_parts) {
        all_candicate_parts_.insert(p);
      }
      // Build response XML using all_objects_
      GenerateRespXml();
    } else if (s.ToString().find("Bucket Doesn't Belong To This User") !=
               std::string::npos) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    } else {
      http_ret_code_ = 500;
    }
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int ListPartsCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}

void ListPartsCmd::GenerateRespXml() {
  // Build response XML
  S3XmlDoc doc("ListPartsResult");

  doc.AppendToRoot("Bucket", bucket_name_);
  doc.AppendToRoot("Key", object_name_);
  doc.AppendToRoot("UploadId", upload_id_);

  S3XmlNode* init_node = doc.AllocateNode("Initiator");
  init_node->AppendNode("ID", slash::sha256(user_name_));
  init_node->AppendNode("DisplayName", user_name_);
  doc.AppendToRoot(init_node);

  S3XmlNode* owner_node = doc.AllocateNode("Owner");
  owner_node->AppendNode("ID", slash::sha256(user_name_));
  owner_node->AppendNode("DisplayName", user_name_);
  doc.AppendToRoot(owner_node);

  doc.AppendToRoot("StorageClass", "STANDARD");
  doc.AppendToRoot("PartNumberMarker", part_num_marker_);
  doc.AppendToRoot("MaxParts", std::to_string(max_parts_));

  bool is_trucated = false;
  int count = 0;
  std::string next_marker;

  for (auto& part : all_candicate_parts_) {
    if (count >= max_parts_) {
      if (max_parts_ > 0) {
        is_trucated = true;
      }
      break;
    }
    if (!part_num_marker_.empty() &&
        part.object_name < part_num_marker_) {
      continue;
    }
    S3XmlNode* part_node = doc.AllocateNode("Part");
    part_node->AppendNode("PartNumber", part.object_name);
    part_node->AppendNode("LastModified", iso8601_time(part.last_modified));
    part_node->AppendNode("ETag", "\"" + part.etag + "\"");
    part_node->AppendNode("Size", std::to_string(part.size));
    doc.AppendToRoot(part_node);

    next_marker = part.object_name;
    count++;
  }

  doc.AppendToRoot("IsTruncated", is_trucated ? "true" : "false");
  doc.AppendToRoot("NextPartNumberMarker", (!is_trucated || count == 0) ? "0" :
                   next_marker);

  doc.ToString(&http_response_xml_);
}
