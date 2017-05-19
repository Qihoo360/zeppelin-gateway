#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "slash/include/slash_string.h"
#include "src/zgwstore/zgw_define.h"
#include "src/s3_cmds/zgw_s3_xml.h"

bool UploadPartCopyPartialCmd::DoInitial() {
  DLOG(INFO) << "UploadPartCopyPartial(DoInitial) - " <<
    req_headers_.at("x-amz-copy-source-range");
  http_response_xml_.clear();
  src_data_block_.clear();
  data_size_ = 0;
  upload_id_ = query_params_.at("uploadId");
  part_number_ = query_params_.at("partNumber");
  md5_ctx_.Init();

  if (!TryAuth()) {
    return false;
  }

  std::string source_path = req_headers_.at("x-amz-copy-source");
  SplitBySecondSlash(source_path, &src_bucket_name_, &src_object_name_);
  if (src_bucket_name_.empty() || src_object_name_.empty()) {
    http_ret_code_ = 400;
    GenerateErrorXml(kInvalidArgument, "x-amz-copy-source");
    return false;
  }

  return true;
}

int UploadPartCopyPartialCmd::ParseRange(const std::string& range, uint64_t data_size,
                                         uint64_t* range_start, uint64_t* range_end) {
  // Check whether range is valid
  // Support sigle range for only
  if (range.find("bytes=") == std::string::npos) {
    return 400;
  }
  std::string range_header = range.substr(6);
  uint64_t start, end = UINT64_MAX;
  int res = sscanf(range_header.c_str(), "%lu-%lu", &start, &end);
  end = std::min(end, data_size - 1);

  if (res > 0 &&
      start <= end) {
    // Valid range
    LOG(INFO) << "Get range: " << start << "-" << end;
    *range_start = start;
    *range_end = end;
    return 206;
  }
  return 416;
}

void UploadPartCopyPartialCmd::ParseBlocksFrom(const std::vector<std::string>& block_indexes) {
  if (data_size_ == 0) {
    http_ret_code_ = 416;
    GenerateErrorXml(kInvalidRange, src_object_.object_name);
    return;
  }
  assert(data_size_ > 0);
  uint64_t range_start = 0;
  uint64_t range_end = data_size_ - 1;

  http_ret_code_ = ParseRange(req_headers_.at("x-amz-copy-source-range"), data_size_,
                              &range_start, &range_end);
  if (http_ret_code_ == 400) {
    GenerateErrorXml(kInvalidArgument, "range");
    return;
  } else if (http_ret_code_ == 416) {
    GenerateErrorXml(kInvalidRange, src_object_.object_name);
    return;
  } else if (http_ret_code_ == 206) {
    // Success partial
    http_ret_code_ = 206;
    data_size_ = range_end - range_start + 1;
  } else {
    // Can't reach here
    assert(true);
  }

  uint64_t partial_start_b, partial_end_b;
  uint64_t needed_size = range_end - range_start + 1;
  char buf[100];
  for (size_t i = 0; i < block_indexes.size(); i++) {
    uint64_t partial_size = 0;
    // Parse all block needed
    uint64_t start_block, end_block, start_byte, data_size;
    int ret = sscanf(block_indexes[i].c_str(), "%lu-%lu(%lu,%lu)",
                     &start_block, &end_block, &start_byte, &data_size);

    // Select block interval index
    if (range_start >= (data_size - start_byte)) {
      range_start -= (data_size - start_byte);
      start_byte = 0;
      // Next block group
      continue;
    }

    partial_start_b = start_block;
    for (uint64_t b = start_block; b <= end_block; b++) {
      uint64_t cur_bsize = std::min(data_size, zgwstore::kZgwBlockSize) - start_byte;
      // Select block
      if (range_start >= cur_bsize) {
        partial_start_b = b + 1;
        range_start -= cur_bsize;
        start_byte = 0;
        // Next block
        continue;
      }

      partial_end_b = b;
      partial_size += cur_bsize - range_start;

      uint64_t remain = std::min(needed_size,
                                 zgwstore::kZgwBlockSize - range_start);
      remain = std::min(remain, data_size);

      blocks_.push(std::make_tuple(b, start_byte + range_start, remain));
      needed_size -= remain;
      start_byte = 0;

      if (needed_size == 0) {
        // The last block group
        sprintf(buf, "%lu-%lu(%lu,%lu)", partial_start_b, partial_end_b,
                range_start, partial_size);
        src_data_block_.push_back(std::string(buf));
        return;
      }

      range_start = 0;
    }

    // Include this block group
    sprintf(buf, "%lu-%lu(%lu,%lu)", partial_start_b, partial_end_b,
            range_start, partial_size);
    src_data_block_.push_back(std::string(buf));
  }
}

void UploadPartCopyPartialCmd::SortBlockIndexes(std::vector<std::string>* block_indexes) {
  std::sort(block_indexes->begin(), block_indexes->end(),
            [](const std::string& a, const std::string& b) {
              return std::atoi(a.substr(0, 5).c_str()) <
                      std::atoi(b.substr(0, 5).c_str());
            });
  std::vector<std::string> tmp(*block_indexes);
  block_indexes->clear();
  for (auto& b : tmp) {
    // Trim sort sign e.g. 00012(3-9)(0,12) -> (3-9)(0,12)
    b = b.substr(5);
    std::vector<std::string> items;
    slash::StringSplit(b, '|', items);
    for (auto& i : items) {
      block_indexes->push_back(i);
    }
  }
}


void UploadPartCopyPartialCmd::DoAndResponse(pink::HttpResponse* resp) {
  uint64_t range_start, range_end;
  std::string virtual_bucket = "__TMPB" + upload_id_ +
    bucket_name_ + "|" + object_name_;

  if (http_ret_code_ == 200) {
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
      }
    } else {
      // Get source object
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
        }
      } else {
        // Parse range
        data_size_ = src_object_.size;
        std::vector<std::string> sorted_block_indexes;
        if (src_object_.upload_id != "_") {
          s = store_->GetMultiBlockSet(src_object_.bucket_name, src_object_.object_name,
                                       src_object_.upload_id, &sorted_block_indexes);
          if (s.IsIOError()) {
            http_ret_code_ = 500;
          }
          // Sort block indexes load from redis set
          SortBlockIndexes(&sorted_block_indexes);
        } else {
          // Sigle block index needn't sorting
          sorted_block_indexes.push_back(src_object_.data_block);
        }
        if (http_ret_code_ == 200) {
          ParseBlocksFrom(sorted_block_indexes);
        }
      }
    }
  }

  // Get partial success, copy to new object
  if (http_ret_code_ == 206) {
    // Initial new_object_
    new_object_.bucket_name = virtual_bucket;
    new_object_.object_name = part_number_;
    new_object_.etag = ""; // Postpone
    new_object_.size = data_size_; // Calc in ParseBlocksFrom(...)
    new_object_.owner = user_name_;
    new_object_.last_modified = 0; // Postpone
    new_object_.storage_class = 0; // Unused
    new_object_.acl = "FULL_CONTROL";
    new_object_.upload_id = upload_id_;
    new_object_.data_block.clear();

    for (auto& db : src_data_block_) {
      new_object_.data_block.append(db + "|");
    }

    // Calc blocks MD5 from blocks_ queue
    std::string block_buffer(zgwstore::kZgwBlockSize, 0);
    while (!blocks_.empty() && status_.ok()) {
      std::string block_num = std::to_string(std::get<0>(blocks_.front()));
      uint64_t start_byte = std::get<1>(blocks_.front());
      uint64_t size = std::get<2>(blocks_.front());
      blocks_.pop();
      status_ = store_->BlockGet(block_num, &block_buffer);
      if (!status_.ok()) {
        http_ret_code_ = 500;
        break;
      }
      md5_ctx_.Update(block_buffer.data() + start_byte, size);
    }
    if (status_.ok()) {
      // TODO (gaodq) zp reference src_data_block_
      new_object_.etag = md5_ctx_.ToString();
      if (new_object_.etag.empty()) {
        new_object_.etag = "_";
      }
      new_object_.last_modified = slash::NowMicros();
      // Write part meta
      status_ = store_->AddObject(new_object_);
      if (!status_.ok()) {
        http_ret_code_ = 500;
      }
    }
  }

  // Success
  if (http_ret_code_ == 206) {
      GenerateRespXml();
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

void UploadPartCopyPartialCmd::GenerateRespXml() {
  S3XmlDoc doc("CopyObjectResult");
  doc.AppendToRoot(doc.AllocateNode("LastModified",
                                    iso8601_time(new_object_.last_modified)));
  doc.AppendToRoot(doc.AllocateNode("ETag", new_object_.etag));

  doc.ToString(&http_response_xml_);
}

int UploadPartCopyPartialCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
