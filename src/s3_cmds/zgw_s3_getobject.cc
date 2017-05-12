#include "src/s3_cmds/zgw_s3_object.h"

#include <glog/logging.h>
#include "slash/include/env.h"

bool GetObjectCmd::DoInitial() {
  block_buf_pos_ = 0;
  start_bytes_ = 0;
  data_size_ = 0;
  block_buffer_.clear();
  range_result_.clear();
  std::queue<uint64_t> empty;
  std::swap(blocks_, empty);
  need_partial_ = req_headers_.count("range");

  return TryAuth();
}

void GetObjectCmd::ParseBlocksFrom(const std::string& blocks_str,
                                   const std::string& range) {
  // Parse all block needed
  uint64_t start_block, end_block;
  int ret = sscanf(blocks_str.c_str(), "%lu-%lu(%lu,%lu)",
                   &start_block, &end_block, &start_bytes_, &data_size_);

  if (need_partial_) {
    // Parse range block needed, return if error exist
    // Check whether range is valid
    // Support one range for only
    if (range.find("bytes=") == std::string::npos) {
      http_ret_code_ = 400;
      GenerateErrorXml(kInvalidArgument, "range");
      return;
    }
    std::string range_header = range.substr(6);
    uint64_t start, end = UINT64_MAX;
    int res = sscanf(range_header.c_str(), "%lu-%lu", &start, &end);
    end = std::min(end, data_size_ - 1);
    if (res > 0 &&
        start <= end &&
        start < data_size_ &&
        end < data_size_) {
      int start_num = (start + start_bytes_) / zgwstore::kZgwBlockSize +
        (((start + start_bytes_) % zgwstore::kZgwBlockSize) ? 1 : 0);
      data_size_ = end - start + 1;
      int need_num = data_size_ / zgwstore::kZgwBlockSize +
        ((data_size_ % zgwstore::kZgwBlockSize) ? 1 : 0);
        
      start_block += start_num;
      end_block = start_block + need_num - 1;
      start_bytes_ = start % zgwstore::kZgwBlockSize;

      char buf[100];
      sprintf(buf, "bytes %lu-%lu/%lu", start, end, object_.size);
      http_ret_code_ = 206;
      range_result_.assign(buf);
    } else {
      http_ret_code_ = 416;
      GenerateErrorXml(kInvalidRange, object_name_);
      return;
    }
  }

  for (uint64_t i = start_block; i <= end_block; i++) {
    blocks_.push(i);
  }
}

void GetObjectCmd::DoAndResponse(pink::HttpResponse* resp) {
  // Get object_ meta
  Status s = store_->GetObject(user_name_, bucket_name_, object_name_,
                               &object_);
  if (s.ok()) {
    http_ret_code_ = 200;
    data_size_ = object_.size;

    // http_ret_code_ and http_response_xml_
    // has been set in ParseBlocksFrom if
    // errors happened
    ParseBlocksFrom(object_.data_block, req_headers_["range"]);
  } else {
    if (s.ToString().find("Bucket Doesn't Belong To This User") !=
        std::string::npos) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    } else if (s.ToString().find("Object Not Found") != std::string::npos) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchKey, object_name_);
    }
  }

  if (http_ret_code_ == 200 ||
      http_ret_code_ == 206) {
    // Success
    if (http_ret_code_ == 206) {
      resp->SetHeaders("Content-Range", range_result_);
    }
    resp->SetHeaders("ETag", object_.etag);
    resp->SetHeaders("Last-Modified", http_nowtime(object_.last_modified));
    resp->SetContentLength(data_size_);
  } else {
    resp->SetContentLength(http_response_xml_.size());
  }
  resp->SetHeaders("Date", http_nowtime(slash::NowMicros()));
  resp->SetStatusCode(http_ret_code_);
}

int GetObjectCmd::DoResponseBody(char* buf, size_t max_size) {
  if (http_ret_code_ != 200 &&
      http_ret_code_ != 206 &&
      !http_response_xml_.empty()) {
    // Error happend, response error xml
    if (max_size < http_response_xml_.size()) {
      memcpy(buf, http_response_xml_.data(), max_size);
      http_response_xml_.assign(http_response_xml_.substr(max_size));
    } else {
      memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
    }
    return std::min(max_size, http_response_xml_.size());
  }
  // Response object data
  if (object_.size == 0) {
    return 0;
  }
  if (block_buffer_.empty() &&
      !blocks_.empty()) {
    std::string block_num = std::to_string(blocks_.front());
    blocks_.pop();
    Status s = store_->BlockGet(block_num, &block_buffer_);
    if (!s.ok()) {
      // Zeppelin error, close the http connection
      return -1;
    }
    block_buf_pos_ = start_bytes_;
    start_bytes_ = 0;
  }
  size_t nwritten = std::min(max_size, block_buffer_.size() - block_buf_pos_);
  memcpy(buf, block_buffer_.data() + block_buf_pos_, nwritten);
  block_buf_pos_ += nwritten;
  if (block_buf_pos_ == block_buffer_.size()) {
    block_buffer_.clear();
  }
  return nwritten;
}
