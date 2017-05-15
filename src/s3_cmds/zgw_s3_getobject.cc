#include "src/s3_cmds/zgw_s3_object.h"

#include <glog/logging.h>
#include "slash/include/env.h"

bool GetObjectCmd::DoInitial() {
  block_buffer_.clear();
  data_size_ = 0;
  range_result_.clear();
  std::queue<std::tuple<uint64_t, uint64_t, uint64_t>> empty;
  std::swap(blocks_, empty);
  need_partial_ = req_headers_.count("range");

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

void GetObjectCmd::ParseBlocksFrom(const std::vector<std::string>& block_indexes) {
  assert(data_size_ > 0);
  uint64_t range_start = 0;
  uint64_t range_end = data_size_ - 1;
  if (need_partial_) {
    http_ret_code_ = ParseRange(query_params_.at("range"), data_size_,
                                &range_start, &range_end);
    if (http_ret_code_ == 400) {
      GenerateErrorXml(kInvalidArgument, "range");
    } else if (http_ret_code_ == 416) {
      GenerateErrorXml(kInvalidRange, object_name_);
    } else if (http_ret_code_ == 206) {
      // Success partial
      char buf[100];
      sprintf(buf, "bytes %lu-%lu/%lu", range_start, range_end, object_.size);
      http_ret_code_ = 206;
      range_result_.assign(buf);
      data_size_ = range_end - range_start + 1;
    } else {
      // Can't reach here
    }
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

void GetObjectCmd::DoAndResponse(pink::HttpResponse* resp) {
  // Get object_ meta
  Status s = store_->GetObject(user_name_, bucket_name_, object_name_,
                               &object_);
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
    data_size_ = object_.size;

    if (data_size_ != 0) {
      http_ret_code_ = 200;

      std::vector<std::string> block_indexes;
      if (object_.upload_id == "_") {
        s = store_->GetMultiBlockSet(bucket_name_, object_name_,
                                     object_.upload_id, &block_indexes);
        if (s.IsIOError()) {
          http_ret_code_ = 500;
        }
      } else {
        block_indexes.push_back(object_.data_block);
      }
      if (http_ret_code_ == 200) {
        ParseBlocksFrom(block_indexes);
      }
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
  if (data_size_ == 0) {
    return 0;
  }

  uint64_t block_size = std::get<2>(blocks_.front());
  if (block_size > max_size) {
    // Waiting for enough buffer
    return 0;
  }
  std::string block_index = std::to_string(std::get<0>(blocks_.front()));
  uint64_t start_byte = std::get<1>(blocks_.front());
  Status s = store_->BlockGet(block_index, &block_buffer_);
  if (!s.ok()) {
    // Zeppelin error, close the http connection
    return -1;
  }
  memcpy(buf, block_buffer_.data() + start_byte, block_size);
  return block_size;
}
