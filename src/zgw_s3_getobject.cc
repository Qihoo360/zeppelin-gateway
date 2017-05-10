#include "src/zgw_s3_object.h"

bool GetObjectCmd::DoInitial() {
  block_buf_pos_ = 0;
  block_buffer_.clear();

  // Get object_ meta
  // Status s = store_->GetObject(bucket_name_, &object_);
  Status s;
  if (s.ok()) {
    http_ret_code_ = 200;
  } else if (s.IsNotFound()) {
    http_ret_code_ = 404;
    // TODO(gaodq) Set xml response
  } else {
    http_ret_code_ = 500;
  }

  return true;
}

void GetObjectCmd::DoAndResponse(pink::HttpResponse* resp) {
  if (http_ret_code_ == 200) {
    // Need reply right now
    resp->SetContentLength(object_.size);
  } else {
    resp->SetContentLength(http_response_xml_.size());
  }

  resp->SetStatusCode(http_ret_code_);
}

int GetObjectCmd::DoResponseBody(char* buf, size_t max_size) {
  if (http_ret_code_ != 200 && !http_response_xml_.empty()) {
    // Error happend
    if (max_size < http_response_xml_.size()) {
      memcpy(buf, http_response_xml_.data(), max_size);
      http_response_xml_.assign(http_response_xml_.substr(max_size));
    } else {
      memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
    }
    return std::min(max_size, http_response_xml_.size());
  }
  // Response object data
  if (block_buffer_.empty()) {
    // Status s = store_->GetBlockFromZp(block_num, &block_buffer_);
    Status s;
    if (!s.ok()) {
      // Zeppelin error, close the http connection
      return -1;
    }
    block_buf_pos_ = 0;
  }
  size_t nwritten = std::min(max_size, block_buffer_.size());
  memcpy(buf, block_buffer_.data() + block_buf_pos_, nwritten);
  block_buf_pos_ += nwritten;
  return nwritten;
}
