#include "src/s3_cmds/zgw_s3_object.h"

#include <algorithm>

#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool CompleteMultiUploadCmd::DoInitial() {
  http_request_xml_.clear();
  http_response_xml_.clear();
  received_parts_info_.clear();
  upload_id_ = query_params_.at("uploadId");
  md5_ctx_.Init();
  request_id_ = md5(bucket_name_ +
                    object_name_ +
                    upload_id_ + 
                    std::to_string(slash::NowMicros()));

  if (!TryAuth()) {
    DLOG(ERROR) << request_id_ <<
      "CompleteMultiUpload(DoInitial) - Auth failed: " << client_ip_port_;
    return false;
  }

  DLOG(INFO) << request_id_ <<
    "CompleteMultiUpload(DoInitial) - " << bucket_name_ << "/" << object_name_ <<
    ", uploadId: " << upload_id_;

  return true;
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
    // Trim quote of etag
    std::string etag_s = etag.value();
    if (!etag_s.empty() && etag_s.at(0) == '"') {
      etag_s.assign(etag_s.substr(1, 32));
    }
    received_parts_info_.push_back(std::make_pair(part_num.value(), etag_s));
  } while (node.NextSibling());
  return true;
}

void CompleteMultiUploadCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    if (!ParseReqXml()) {
      http_ret_code_ = 400;
      GenerateErrorXml(kMalformedXML);
    }

    std::string virtual_bucket = "__TMPB" + upload_id_ +
      bucket_name_ + "|" + object_name_;
    std::string new_data_blocks;
    uint64_t data_size = 0;
    std::vector<zgwstore::Object> stored_parts;

    DLOG(INFO) << request_id_ <<
      "CompleteMultiUpload(DoAndResponse) - virtual_bucket: " << virtual_bucket;

    Status s = store_->Lock();
    if (s.ok()) {
      DLOG(INFO) << request_id_ <<
        "CompleteMultiUpload(DoAndResponse) - Lock success";

      if (http_ret_code_ == 200) {
        s = store_->ListObjects(user_name_, virtual_bucket, &stored_parts);
        if (!s.ok()) {
          if (s.ToString().find("Bucket Doesn't Belong To This User") !=
              std::string::npos) {
            http_ret_code_ = 404;
            GenerateErrorXml(kNoSuchUpload, upload_id_);
          } else {
            http_ret_code_ = 500;
            LOG(ERROR) << request_id_ <<
              "CompleteMultiUpload(DoAndResponse) - ListVirtObjects " <<
              virtual_bucket << "error: " << s.ToString();
          }
        } else {
          // Complete multiupload
          if (received_parts_info_.size() != stored_parts.size()) {
            http_ret_code_ = 400;
            GenerateErrorXml(kInvalidPart);
          } else  {
            // sort stored parts
            std::sort(stored_parts.begin(), stored_parts.end(),
                      [](const zgwstore::Object& a, const zgwstore::Object& b) {
                        return std::atoi(a.object_name.c_str()) <
                                std::atoi(b.object_name.c_str());
                      });
            for (size_t i = 0 ; i < received_parts_info_.size(); i++) {
              bool found_part = false;
              for (auto& stored_part : stored_parts) {
                if (received_parts_info_[i].first == stored_part.object_name) {
                  found_part = true;
                  break;
                }
              }
              if (!found_part) {
                http_ret_code_ = 400;
                GenerateErrorXml(kInvalidPart);
                break;
              }
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

              // AddMultiBlockSet
              // Add  sort sign
              char buf[100];
              sprintf(buf, "%05d", std::atoi(stored_parts[i].object_name.c_str()));
              s = store_->AddMultiBlockSet(bucket_name_, object_name_, upload_id_,
                                           std::string(buf) + stored_parts[i].data_block);
              if (!s.ok()) {
                http_ret_code_ = 500;
                LOG(ERROR) << request_id_ <<
                  "CompleteMultiUpload(DoAndResponse) - AddMultiBlockSet" <<
                  virtual_bucket << "/" << stored_parts[i].object_name << " " <<
                  upload_id_ << " error: " << s.ToString();
              }
            }
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

        // Write meta
        Status s = store_->AddObject(new_object_, false);
        if (!s.ok()) {
          http_ret_code_ = 500;
          LOG(ERROR) << request_id_ <<
            "CompleteMultiUpload(DoAndResponse) - AddObject " << bucket_name_ <<
            "/" << object_name_ << "error: " << s.ToString();
        } else {
          DLOG(INFO) << request_id_ <<
            "CompleteMultiUpload(DoAndResponse) - AddObject " << bucket_name_ <<
            "/" << object_name_ << "success, cleaning...";
          for (auto& p : stored_parts) {
            s = store_->DeleteObject(user_name_, virtual_bucket, p.object_name, false);
            if (s.IsIOError()) {
              http_ret_code_ = 500;
              LOG(ERROR) << request_id_ <<
                "CompleteMultiUpload(DoAndResponse) - DeleteVirtObject " <<
                virtual_bucket << "/" << p.object_name << " error: " << s.ToString();
            }
          }
          if (http_ret_code_ == 200) {
            s = store_->DeleteBucket(user_name_, virtual_bucket, false);
            if (s.IsIOError()) {
              http_ret_code_ = 500;
              LOG(ERROR) << request_id_ <<
                "CompleteMultiUpload(DoAndResponse) - DeleteVirtBucket " <<
                virtual_bucket << " error: " << s.ToString();
            }
          }
        }
      }

      s = store_->UnLock();
    }
    if (!s.ok()) {
      http_ret_code_ = 500;
      LOG(ERROR) << request_id_ <<
        "CompleteMultiUpload(DoAndResponse) - Lock or Unlock error:" <<
        s.ToString();
    }
    DLOG(INFO) << request_id_ <<
      "CompleteMultiUpload(DoAndResponse) - UnLock success";

    if (http_ret_code_ == 200) {
      DLOG(INFO) << "CompleteMultiUpload(DoAndResponse) - Clean OK";
      // Build response XML
      GenerateRespXml();
    }
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

void CompleteMultiUploadCmd::GenerateRespXml() {
  S3XmlDoc doc("CompleteMultipartUploadResult");
  doc.AppendToRoot(doc.AllocateNode("Bucket", bucket_name_));
  doc.AppendToRoot(doc.AllocateNode("Key", object_name_));
  doc.AppendToRoot(doc.AllocateNode("ETag", "\"" + new_object_.etag + "\""));
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
