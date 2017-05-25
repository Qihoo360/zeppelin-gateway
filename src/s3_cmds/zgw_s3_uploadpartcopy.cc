#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "slash/include/slash_string.h"
#include "src/zgwstore/zgw_define.h"
#include "src/s3_cmds/zgw_s3_xml.h"

bool UploadPartCopyCmd::DoInitial() {
  http_response_xml_.clear();

  if (!TryAuth()) {
    DLOG(ERROR) <<
      "UploadPartCopy(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  std::string source_path = req_headers_.at("x-amz-copy-source");
  SplitBySecondSlash(source_path, &src_bucket_name_, &src_object_name_);
  if (src_bucket_name_.empty() || src_object_name_.empty()) {
    http_ret_code_ = 400;
    GenerateErrorXml(kInvalidArgument, "x-amz-copy-source");
    return false;
  }

  upload_id_ = query_params_.at("uploadId");
  part_number_ = query_params_.at("partNumber");

  request_id_ = md5(bucket_name_ +
                    object_name_ +
                    upload_id_ + 
                    part_number_ +
                    std::to_string(slash::NowMicros()));

  DLOG(INFO) << request_id_ << " " <<
    "UploadPartCopy(DoInitial) - " << bucket_name_ << "/" << object_name_ <<
    ", uploadId: " << upload_id_ << " part_number: " << part_number_;
  return true;
}

void UploadPartCopyCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    // Get src_object_ meta
    Status s = store_->GetObject(user_name_, src_bucket_name_, src_object_name_,
                                 &src_object_);
    if (!s.ok()) {
      if (s.ToString().find("Bucket Doesn't Belong To This User") !=
          std::string::npos) {
        http_ret_code_ = 404;
        GenerateErrorXml(kNoSuchBucket, bucket_name_);
      } else if (s.ToString().find("Object Not Found") != std::string::npos) {
        http_ret_code_ = 404;
        GenerateErrorXml(kNoSuchKey, object_name_);
      } else if (s.IsIOError()) {
        http_ret_code_ = 500;
        LOG(ERROR) << request_id_ << " " <<
          "UploadPartCopy(DoAndResponse) - GetSrcObject failed: " <<
          src_bucket_name_ << "/" << src_object_name_ << " " << s.ToString();
      }
    } else {
      std::string virtual_bucket = "__TMPB" + upload_id_ +
        bucket_name_ + "|" + object_name_;

      // Initial new_object_
      new_object_.bucket_name = virtual_bucket;
      new_object_.object_name = part_number_;
      new_object_.etag = src_object_.etag;
      new_object_.size = src_object_.size;
      new_object_.owner = user_name_;
      new_object_.last_modified = slash::NowMicros();
      new_object_.storage_class = 0; // Unused
      new_object_.acl = "FULL_CONTROL";
      new_object_.upload_id = upload_id_;
      new_object_.data_block = src_object_.data_block;

      // Check uploadId
      zgwstore::Bucket dummy_bk;
      Status s = store_->GetBucket(user_name_, virtual_bucket, &dummy_bk);
      if (!s.ok()) {
        if (s.ToString().find("Bucket Doesn't Belong To This User") ||
            s.ToString().find("Bucket Not Found")) {
          http_ret_code_ = 404;
          GenerateErrorXml(kNoSuchUpload, upload_id_);
        } else {
          http_ret_code_ = 500;
          LOG(ERROR) << request_id_ << " " <<
            "UploadPartCopy(DoAndResponse) - GetVirtBucket failed: " <<
            virtual_bucket << " " << s.ToString();
        }
      } else {
        s = AddBlocksRef(src_object_.upload_id, src_object_.data_block);
        if (!s.ok()) {
          http_ret_code_ = 500;
          LOG(ERROR) << request_id_ << " " <<
            "UploadPartCopy(DoAndResponse) - AddBlocksRef failed: " << s.ToString();
        } else {

          // Add part meta
          s = store_->AddObject(new_object_, false);
          if (!s.ok()) {
            http_ret_code_ = 500;
            LOG(ERROR) << request_id_ << " " <<
              "UploadPartCopy(DoAndResponse) - AddObject failed: " << s.ToString();
          }
        }

        // Success
        GenerateRespXml();
      }
    }
  }

  g_zgw_monitor->AddApiRequest(kUploadPartCopy, http_ret_code_);
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

void UploadPartCopyCmd::GenerateRespXml() {
  assert(http_ret_code_ == 200);
  S3XmlDoc doc("CopyObjectResult");
  doc.AppendToRoot(doc.AllocateNode("LastModified",
                                    iso8601_time(new_object_.last_modified)));
  doc.AppendToRoot(doc.AllocateNode("ETag", new_object_.etag));

  doc.ToString(&http_response_xml_);
}

int UploadPartCopyCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}

Status UploadPartCopyCmd::AddBlocksRef(const std::string& upload_id,
                                       const std::string& data_blocks) {
  Status s;
  std::vector<std::string> block_indexes;
  if (upload_id != "_") { // Has multipart
    size_t pos = data_blocks.find("|");
    assert(pos != std::string::npos);
    std::string bkname = data_blocks.substr(32, pos - 32);
    std::string obname = data_blocks.substr(pos + 1);
    s = store_->GetMultiBlockSet(bkname, obname,
                                 upload_id, &block_indexes);
    if (s.IsIOError()) {
      http_ret_code_ = 500;
      LOG(ERROR) << request_id_ << " " <<
        "PutObjectCopy(DoAndResponse) - GetMultiBlockSet Error: " << data_blocks
        << s.ToString();
    }
    std::vector<std::string> tmp;
    tmp.swap(block_indexes);
    for (auto& b : tmp) {
      // Trim sort sign e.g. 00012(3-9)(0,12) -> (3-9)(0,12)
      b = b.substr(5);
      std::vector<std::string> items;
      slash::StringSplit(b, '|', items);
      for (auto& i : items) {
        block_indexes.push_back(i);
      }
    }
  } else {
    // Sigle block index needn't sorting
    block_indexes.push_back(data_blocks);
  }

  s = store_->Lock();
  if (!s.ok()) {
    return s;
  }
  for (auto& blockg : block_indexes) {
    uint64_t start_block, end_block, start_byte, data_size;
    int ret = sscanf(blockg.c_str(), "%lu-%lu(%lu,%lu)",
                     &start_block, &end_block, &start_byte, &data_size);
    for (uint64_t b = start_block; b <= end_block; b++) {
      Status s = store_->BlockRef(std::to_string(b));
      if (!s.ok()) {
      LOG(ERROR) << request_id_ << " " <<
        "PutObjectCopy(DoAndResponse) - BlockRef Error: " << data_blocks;
        return s;
      }
    }
  }
  s = store_->UnLock();
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}
