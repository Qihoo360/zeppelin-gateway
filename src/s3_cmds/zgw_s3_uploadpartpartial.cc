#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"

bool UploadPartCopyPartialCmd::DoInitial() {
  http_response_xml_.clear();
  data_size_ = 0;
  range_ = query_params_.at("x-amz-copy-source-range");
  upload_id_ = query_params_.count("uploadId");

  std::string source_path = req_headers_.at("x-amz-copy-source");
  SplitBySecondSlash(source_path, &src_bucket_name_, &src_object_name_);
  if (src_bucket_name_.empty() || src_object_name_.empty()) {
    http_ret_code_ = 400;
    GenerateErrorXml(kInvalidArgument, "x-amz-copy-source");
    return false;
  }

  return TryAuth();
}

static int ParseRange(const std::string& range, uint64_t data_size,
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
  assert(data_size_ > 0);
  uint64_t range_start = 0;
  uint64_t range_end = data_size_ - 1;

  http_ret_code_ = ParseRange(query_params_.at("range"), data_size_,
                              &range_start, &range_end);
  if (http_ret_code_ == 400) {
    GenerateErrorXml(kInvalidArgument, "range");
  } else if (http_ret_code_ == 416) {
    GenerateErrorXml(kInvalidRange, object_name_);
  } else if (http_ret_code_ == 206) {
    // Success
  } else {
    // Can't reach here
  }

  uint64_t needed_size = range_end - range_start + 1;
  for (size_t i = 0; i < block_indexes.size(); i++) {
    // Parse all block needed
    uint64_t start_block, end_block, start_byte, data_size;
    int ret = sscanf(block_indexes[i].c_str(), "%lu-%lu(%lu,%lu)",
                     &start_block, &end_block, &start_byte, &data_size);

    if (range_start >= data_size) {
      continue;
    }

    for (uint64_t b = start_block; b <= end_block; b++) {
      uint64_t end_of_block = (b - start_block + 1) * zgwstore::kZgwBlockSize;
      if (range_start >= end_of_block) {
        continue;
      }
      uint64_t remain = std::min(needed_size, zgwstore::kZgwBlockSize);
      blocks_.push(std::make_tuple(b, start_byte + range_start, remain));
      needed_size -= remain;
      start_byte = 0;
    }

    if (needed_size == 0) {
      break;
    }

    range_start = 0;
  }
}

void UploadPartCopyPartialCmd::DoAndResponse(pink::HttpResponse* resp) {
  uint64_t range_start, range_end;
  // Check uploadId
  if (http_ret_code_ == 200) {
    std::string virtual_bucket = "__TMPB" + upload_id_ +
      bucket_name_ + "|" + object_name_;
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
        data_size_ = src_object_.size;
        if (data_size_ != 0) {
          std::vector<std::string> block_indexes;
          if (src_object_.upload_id == "_") {
            s = store_->GetMultiBlockSet(bucket_name_, object_name_,
                                         src_object_.upload_id, &block_indexes);
            if (s.IsIOError()) {
              http_ret_code_ = 500;
            }
          } else {
            block_indexes.push_back(src_object_.data_block);
          }
          if (http_ret_code_ == 200) {
            ParseBlocksFrom(block_indexes);
          }
        }
      }
    }
  }

  // Get partial success
  if (http_ret_code_ == 206) {
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
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
