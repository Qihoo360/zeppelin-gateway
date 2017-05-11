#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"

bool PutObjectCmd::DoInitial() {
  Timer t("PutObjectCmd: DoInitial -");
  http_response_xml_.clear();
  md5_ctx_.Init();

  size_t data_size = std::stoul(req_headers_["content-length"]);
  size_t m = data_size % zgwstore::kZgwBlockSize;
  block_count_ = data_size / zgwstore::kZgwBlockSize + (m > 0 ? 1 : 0);

  if (!TryAuth()) {
    return false;
  }

  // Initial new_object_
  new_object_.bucket_name = bucket_name_;
  new_object_.object_name = object_name_;
  new_object_.etag = ""; // Postpone
  new_object_.size = data_size;
  new_object_.owner = user_name_;
  new_object_.last_modified = 0; // Postpone
  new_object_.acl = "FULL_CONTROL";
  new_object_.upload_id = "_"; // Doesn't need
  new_object_.data_block = ""; // Postpone

    std::string data_blocks;
  Status s = store_->AllocateId(user_name_, bucket_name_, object_name_,
                                block_count_, &block_end_);
  if (s.ok()) {
    block_start_ = block_end_ + 1 - block_count_;
    http_ret_code_ = 200;
    char buf[100];
    sprintf(buf, "%lu-%lu(0,%lu)", block_start_, block_end_, data_size);
    new_object_.data_block = std::string(buf);
  } else if (s.ToString().find("Bucket NOT Exists") != std::string::npos) {
    http_ret_code_ = 404;
    GenerateErrorXml(kNoSuchBucket, bucket_name_);
    return false;
    // Set xml response
  } else {
    http_ret_code_ = 500;
    return false;
  }

  return true;
}

void PutObjectCmd::DoReceiveBody(const char* data, size_t data_size) {
  Timer t("PutObjectCmd: DoReceiveBody -");
  if (http_ret_code_ != 200 ||
      block_start_ > block_end_) {
    // LOG WARNNING
    return;
  }

  char* buf_pos = const_cast<char*>(data);
  size_t remain_size = data_size;

  while (remain_size > 0) {
    size_t nwritten = std::min(remain_size, zgwstore::kZgwBlockSize);
    status_ = store_->BlockSet(std::to_string(block_start_++),
                               std::string(buf_pos, nwritten));
    if (status_.ok()) {
      // md5_ctx_.Update(data, data_size);
    }

    remain_size -= nwritten;
    buf_pos += nwritten;
  }
}

void PutObjectCmd::DoAndResponse(pink::HttpResponse* resp) {
  Timer t("PutObjectCmd: DoAndResponse -");
  if (http_ret_code_ != 200) {
    // Need reply right now
  } else if (!status_.ok()) {
    // Error happend while transmiting to zeppelin
    http_ret_code_ = 500;
  } else {
    // Write meta
    new_object_.etag = md5_ctx_.ToString();
    if (new_object_.etag.empty()) {
      new_object_.etag = "_";
    }
    new_object_.last_modified = slash::NowMicros();

    status_ = store_->AddObject(new_object_);
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
