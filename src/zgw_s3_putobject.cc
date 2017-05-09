#include "src/zgw_s3_object.h"

bool PutObjectCmd::DoInitial() {
  size_t data_size = std::stoul(req_headers_["content-length"]);
  size_t m = data_size % kZgwBlockSize;
  block_count_ = data_size / kZgwBlockSize + (m > 0 ? 1 : 0);

  // Initial new_object_

  std::string data_blocks;
  // Status s = store_->GetDataBlocks(bucket_name_, block_count_, &block_start_);
  Status s;
  if (s.ok()) {
    block_end_ = block_start_ + block_count_ - 1;
    http_ret_code_ = 200;
    return true;
  } else if (s.IsNotFound()) {
    http_ret_code_ = 404;
    // Set xml response
  } else {
    http_ret_code_ = 500;
  }

  // Needn't reply
  return false;
}

void PutObjectCmd::DoReceiveBody(const char* data, size_t data_size) {
  if (block_start_ > block_end_) {
    // LOG WARNNING
    return;
  }

  char* buf_pos = const_cast<char*>(data);
  size_t remain_size = data_size;

  while (remain_size > kZgwBlockSize) {
    size_t nwritten = std::min(remain_size, kZgwBlockSize);
    // status_ = store_->SaveBlockToZp(current_block_++, buf_pos, nwritten);
    remain_size -= nwritten;
    buf_pos += nwritten;
  }
}

void PutObjectCmd::DoAndResponse(pink::HttpResponse* resp) {
  if (http_ret_code_ != 200) {
    // Need reply right now
  } else if (!status_.ok()) {
    // Error happend while transmiting to zeppelin
    http_ret_code_ = 500;
  } else {
    // Write meta
    // status_ = store_->AddObject(bucket_name_, new_object_);
    if (!status_.ok()) {
      http_ret_code_ = 500;
    }
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int PutObjectCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
