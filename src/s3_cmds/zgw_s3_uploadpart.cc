#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"
#include "src/s3_cmds/zgw_s3_xml.h"

bool UploadPartCmd::DoInitial() {
  http_response_xml_.clear();
  md5_ctx_.Init();

  size_t data_size = std::stoul(req_headers_["content-length"]);
  size_t m = data_size % zgwstore::kZgwBlockSize;
  block_count_ = data_size / zgwstore::kZgwBlockSize + (m > 0 ? 1 : 0);

  if (!TryAuth()) {
    return false;
  }

  std::string upload_id = query_params_.at("uploadId");
  std::string part_number = query_params_.at("partNumber");
  std::string virtual_bucket = "__TMPB" + upload_id +
    bucket_name_ + "|" + object_name_;

  DLOG(INFO) << "UploadPart(DoInitial) - virtual_bucket: " << virtual_bucket
    << " upload_id: " << upload_id << " part_number: " << part_number;

  // Initial new_object_part_
  new_object_part_.bucket_name = virtual_bucket;
  new_object_part_.object_name = part_number;
  new_object_part_.etag = ""; // Postpone
  new_object_part_.size = data_size;
  new_object_part_.owner = user_name_;
  new_object_part_.last_modified = 0; // Postpone
  new_object_part_.storage_class = 0; // Unused
  new_object_part_.acl = "FULL_CONTROL";
  new_object_part_.upload_id = upload_id;
  new_object_part_.data_block = ""; // Postpone

  std::string data_blocks;
  Status s = store_->AllocateId(user_name_, virtual_bucket, part_number,
                                block_count_, &block_end_);
  if (s.ok()) {
    block_start_ = block_end_ - block_count_;
    http_ret_code_ = 200;
    char buf[100];
    sprintf(buf, "%lu-%lu(0,%lu)", block_start_, block_end_ - 1, data_size);
    new_object_part_.data_block.assign(buf);
    DLOG(INFO) << "UploadPart(DoInitial) - AllocateId: " << buf;
  } else if (s.ToString().find("Bucket NOT Exists") != std::string::npos ||
             s.ToString().find("Bucket Doesn't Belong To This User") !=
             std::string::npos) {
    http_ret_code_ = 404;
    GenerateErrorXml(kNoSuchUpload, upload_id);
    return false;
  } else {
    http_ret_code_ = 500;
    LOG(ERROR) << "UploadPart(DoInitial) - AllocateId error: " << s.ToString();
    return false;
  }

  return true;
}

void UploadPartCmd::DoReceiveBody(const char* data, size_t data_size) {
  if (http_ret_code_ != 200) {
    return;
  }

  char* buf_pos = const_cast<char*>(data);
  size_t remain_size = data_size;

  while (remain_size > 0) {
    if (block_start_ >= block_end_) {
      // LOG WARNING
      LOG(WARNING) << "PutObject Block error";
      return;
    }
    size_t nwritten = std::min(remain_size, zgwstore::kZgwBlockSize);
    status_ = store_->BlockSet(std::to_string(block_start_++),
                               std::string(buf_pos, nwritten));
    if (status_.ok()) {
      md5_ctx_.Update(buf_pos, nwritten);
    } else {
      http_ret_code_ = 500;
      LOG(ERROR) << "UploadPart(DoReceiveBody) - BlockSet error: " <<
        status_.ToString();
      // TODO (gaodq) delete data already saved
    }

    remain_size -= nwritten;
    buf_pos += nwritten;
  }
}

void UploadPartCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    if (!status_.ok()) {
      // Error happend while transmiting to zeppelin
      http_ret_code_ = 500;
      LOG(ERROR) << "UploadPart(DoAndResponse) - BlockSet error: " <<
        status_.ToString();
    } else {
      // Write meta
      new_object_part_.etag = md5_ctx_.ToString();
      LOG(INFO) << "UploadPart(DoAndResponse) - MD5: " << new_object_part_.etag;
      if (new_object_part_.etag.empty()) {
        new_object_part_.etag = "_";
      }
      new_object_part_.last_modified = slash::NowMicros();

      DLOG(INFO) << "UploadPart(DoAndResponse) - Lock success";
      status_ = store_->AddObject(new_object_part_);
      if (!status_.ok()) {
        http_ret_code_ = 500;
        LOG(ERROR) << "UploadPart(DoAndResponse) - AddObject error: " <<
          status_.ToString();
      } else {
        DLOG(INFO) << "UploadPart(DoAndResponse) - UnLock success";
        resp->SetHeaders("Last-Modified", http_nowtime(new_object_part_.last_modified));
        resp->SetHeaders("ETag", "\"" + new_object_part_.etag + "\"");
      }
    }
  }

  DLOG(INFO) << "UploadPart(DoAndResponse) - http code: " << http_ret_code_;
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int UploadPartCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
