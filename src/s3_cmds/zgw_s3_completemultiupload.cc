#include "src/s3_cmds/zgw_s3_object.h"

#include <algorithm>

#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool CompleteMultiUploadCmd::DoInitial() {
  http_request_xml_.clear();
  http_response_xml_.clear();
  upload_id_ = query_params_.at("uploadId");
  md5_ctx_.Init();

  return TryAuth();
}

void CompleteMultiUploadCmd::DoReceiveBody(const char* data, size_t data_size) {
  http_request_xml_.append(data, data_size);
}

bool CompleteMultiUploadCmd::ParseReqXml() {
  S3XmlDoc doc;
  if (!doc.ParseFromString(http_request_xml_)) {
    return false;
  }
  S3XmlNode root;
  if (!doc.FindFirstNode("CompleteMultipartUpload", &root)) {
    return false;
  }
  S3XmlNode node;
  if (!root.FindFirstNode("Part", &node)) {
    return false;
  }
  do {
    S3XmlNode part_num, etag;
    if (!node.FindFirstNode("PartNumber", &part_num) ||
        !node.FindFirstNode("ETag", &etag)) {
      return false;
    }
    received_parts_info_.push_back(std::make_pair(part_num.value(), etag.value()));
  } while (node.NextSibling());
  return true;
}

void CompleteMultiUploadCmd::DoAndResponse(pink::HttpResponse* resp) {
  if (!ParseReqXml()) {
    http_ret_code_ = 400;
    GenerateErrorXml(kMalformedXML);
  }

  std::string virtual_bucket = "__TMPB" + upload_id_;
  std::string new_data_blocks;
  int data_size = 0;
  std::vector<zgwstore::Object> stored_parts;

  if (http_ret_code_ == 200) {
    Status s = store_->ListObjects(user_name_, virtual_bucket, &stored_parts);
    if (!s.ok()) {
      if (s.ToString().find("Bucket Doesn't Belong To This User") !=
          std::string::npos) {
        http_ret_code_ = 404;
        GenerateErrorXml(kNoSuchUpload, upload_id_);
      } else {
        http_ret_code_ = 500;
      }
    }

    if (received_parts_info_.size() != stored_parts.size()) {
      http_ret_code_ = 400;
      GenerateErrorXml(kInvalidPart);
    } else  {
      // sort stored parts
      std::sort(stored_parts.begin(), stored_parts.end(),
                [](const zgwstore::Object& a, const zgwstore::Object& b) {
                return a.object_name < b.object_name;
                });
      for (size_t i = 0 ; i < received_parts_info_.size(); i++) {
        if (received_parts_info_[i].first != stored_parts[i].object_name) {
          http_ret_code_ = 400;
          GenerateErrorXml(kInvalidPartOrder);
          break;
        }
        if (received_parts_info_[i].second != stored_parts[i].etag) {
          http_ret_code_ = 400;
          GenerateErrorXml(kInvalidPart);
          break;
        }
        md5_ctx_.Update(stored_parts[i].etag);
        data_size += stored_parts[i].size;
      }
    }
  }

  if (http_ret_code_ == 200) {
    // Initial new_object_
    new_object_.bucket_name = bucket_name_;
    new_object_.object_name = object_name_;
    new_object_.etag = md5_ctx_.ToString() + "-" +
      std::to_string(stored_parts.size());
    new_object_.size = data_size;
    new_object_.owner = user_name_;
    new_object_.last_modified = slash::NowMicros();
    new_object_.storage_class = 0; // Unused
    new_object_.acl = "FULL_CONTROL";
    new_object_.upload_id = upload_id_;
    new_object_.data_block = upload_id_ + bucket_name_ + "|" + object_name_;

    Status s = store_->AddObject(new_object_);
    if (!s.ok()) {
      http_ret_code_ = 500;
    } else {
      for (auto& p : stored_parts) {
        s = store_->DeleteObject(user_name_, virtual_bucket, p.object_name);
        if (s.IsIOError()) {
          http_ret_code_ = 500;
        }
      }
      if (http_ret_code_ == 200) {
        s = store_->DeleteBucket(user_name_, virtual_bucket);
        if (s.IsIOError()) {
          http_ret_code_ = 500;
        }
      }
    }
  }

  if (http_ret_code_ == 200) {
    // Build response XML
    GenerateRespXml();
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

void CompleteMultiUploadCmd::GenerateRespXml() {
  S3XmlDoc doc("CompleteMultipartUploadResult");
  doc.AppendToRoot(doc.AllocateNode("Bucket", bucket_name_));
  doc.AppendToRoot(doc.AllocateNode("Key", object_name_));
  doc.AppendToRoot(doc.AllocateNode("ETag", new_object_.etag));
  doc.ToString(&http_response_xml_);
}

int CompleteMultiUploadCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }
  return std::min(max_size, http_response_xml_.size());
}
