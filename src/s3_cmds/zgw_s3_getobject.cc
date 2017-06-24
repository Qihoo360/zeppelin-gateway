#include "src/s3_cmds/zgw_s3_object.h"

#include <vector>

#include <glog/logging.h>
#include "slash/include/env.h"
#include "slash/include/slash_string.h"

bool GetObjectCmd::DoInitial() {
  block_buffer_.clear();
  data_size_ = 0;
  range_result_.clear();
  std::queue<std::tuple<uint64_t, uint64_t, uint64_t>> empty;
  std::swap(blocks_, empty);
  need_partial_ = req_headers_.count("range");
  request_id_ = md5(bucket_name_ +
                    object_name_ +
                    std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(INFO) << request_id_ << " " <<
      "GetObject(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "GetObject(DoInitial) - " << bucket_name_ << "/" << object_name_;
  return true;
}

int GetObjectCmd::ParseRange(const std::string& range, uint64_t data_size,
                             uint64_t* range_start, uint64_t* range_end) {
  // Check whether range is valid
  // Support sigle range for only
  if (range.find("bytes=") == std::string::npos) {
    return 400;
  }
  std::string range_header = range.substr(6);
  int64_t start = 0;
  int64_t end = INT64_MAX;
  int res = sscanf(range_header.c_str(), "%ld-%ld", &start, &end);
  end = std::min(data_size - 1, static_cast<uint64_t>(end));
  if (res == 1 && start < 0) {
    start = data_size + start;
    end = data_size - 1;
  }

  if (res > 0 &&
      start <= end) {
    // Valid range
    DLOG(INFO) << request_id_ << " " <<
      "Get " << bucket_name_ << "/" << object_name_  <<
      "range: " << start << "-" << end;
    *range_start = start;
    *range_end = end;
    return 206;
  }
  return 416;
}

void GetObjectCmd::ParseBlocksFrom(const std::vector<std::string>& block_indexes) {
  if (data_size_ == 0) {
    if (need_partial_) {
      http_ret_code_ = 416;
      GenerateErrorXml(kInvalidRange, object_name_);
      return;
    } else {
      return;
    }
  }
  assert(data_size_ > 0);
  uint64_t range_start = 0;
  uint64_t range_end = data_size_ - 1;
  if (need_partial_) {
    http_ret_code_ = ParseRange(req_headers_.at("range"), data_size_,
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

    // Select block interval index
    if (range_start >= data_size) {
      range_start -= data_size;
      // Next block group
      continue;
    }

    uint64_t passed_dsize = 0;
    for (uint64_t b = start_block; b <= end_block; b++) {
      uint64_t cur_bsize = std::min(data_size - passed_dsize, zgwstore::kZgwBlockSize - start_byte);
      // Select block
      if (range_start >= cur_bsize) {
        range_start -= cur_bsize;
        passed_dsize += cur_bsize;
        start_byte = 0;
        // Next block
        continue;
      }

      uint64_t remain = std::min(std::min(needed_size, cur_bsize),
                                 zgwstore::kZgwBlockSize - range_start);

      // First block choosed, start_byte maybe not zero
      blocks_.push(std::make_tuple(b, start_byte + range_start, remain));
      needed_size -= remain;
      start_byte = 0;

      if (needed_size == 0) {
        return;
      }

      range_start = 0;
    }
  }
}

void GetObjectCmd::SortBlockIndexes(std::vector<std::string>* block_indexes) {
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

void GetObjectCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
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
      } else {
        http_ret_code_ = 500;
        LOG(ERROR) << request_id_ << " " <<
          "GetObject(DoAndResponse) - GetObject Error" << bucket_name_ << "/" <<
          object_name_ << s.ToString();
      }
    } else {
      data_size_ = object_.size;

      std::vector<std::string> sorted_block_indexes;
      if (object_.upload_id != "_") {
        std::string data_block = object_.data_block;
        size_t pos = data_block.find("|");
        assert(pos != std::string::npos);
        std::string bkname = data_block.substr(32, pos - 32);
        std::string obname = data_block.substr(pos + 1);
        s = store_->GetMultiBlockSet(bkname, obname,
                                     object_.upload_id, &sorted_block_indexes);
        if (s.IsIOError()) {
          http_ret_code_ = 500;
          LOG(ERROR) << request_id_ << " " <<
            "GetObject(DoAndResponse) - GetMultiBlockSet Error: " << data_block
            << s.ToString();
        }
        // Sort block indexes load from redis set
        SortBlockIndexes(&sorted_block_indexes);
      } else {
        // Sigle block index needn't sorting
        sorted_block_indexes.push_back(object_.data_block);
      }
      ParseBlocksFrom(sorted_block_indexes);
    }

    if (http_ret_code_ == 200 ||
        http_ret_code_ == 206) {
      // Success
      if (http_ret_code_ == 206) {
        resp->SetHeaders("Content-Range", range_result_);
      }
      resp->SetHeaders("ETag", "\"" + object_.etag + "\"");
      resp->SetHeaders("Last-Modified", http_nowtime(object_.last_modified));
      resp->SetContentLength(data_size_);

      DLOG(INFO) << request_id_ << " " <<
        "GetObject(DoAndResponse) - " << bucket_name_ <<
        "/" << object_name_ << " Size: " << data_size_;
    }
  }
  if (http_ret_code_ != 206 &&
      http_ret_code_ != 200) {
    // Response error xml
    resp->SetContentLength(http_response_xml_.size());
  }
  g_zgw_monitor->AddApiRequest(kGetObject, http_ret_code_);
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
  blocks_.pop();
  Status s = store_->BlockGet(block_index, &block_buffer_);
  if (!s.ok()) {
    // Zeppelin error, close the http connection
    LOG(ERROR) << request_id_ << " " <<
      "GetObject(DoResponseBody) - BlockGet: " << block_index << s.ToString();
    return -1;
  }
  memcpy(buf, block_buffer_.data() + start_byte, block_size);
  g_zgw_monitor->AddBucketTraffic(bucket_name_, block_size);
  data_size_ -= block_size; // Has written
  if (data_size_ == 0) {
    DLOG(INFO) << request_id_ << " " <<
      "GetObject(DoResponseBody) - Complete " << bucket_name_ << "/"
      << object_name_ << " Size: " << object_.size;
  }
  return block_size;
}
