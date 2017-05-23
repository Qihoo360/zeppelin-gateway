#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool PutObjectCopyCmd::DoInitial() {
  std::string source_path = req_headers_.at("x-amz-copy-source");

  if (!TryAuth()) {
    DLOG(ERROR) <<
      "PutObjectCopy(DoInitial) - Auth failed: " << client_ip_port_;
    g_zgw_monitor->AddAuthFailed();
    return false;
  }

  SplitBySecondSlash(source_path, &src_bucket_name_, &src_object_name_);
  if (src_bucket_name_.empty() || src_object_name_.empty()) {
    http_ret_code_ = 400;
    GenerateErrorXml(kInvalidArgument, "x-amz-copy-source");
    return false;
  }

  request_id_ = md5(bucket_name_ +
                    object_name_ +
                    src_bucket_name_ +
                    src_object_name_ +
                    std::to_string(slash::NowMicros()));

  DLOG(INFO) << request_id_ << " " <<
    "PutObjectCopy(DoInitial) - " << src_bucket_name_ << "/" << src_object_name_ <<
    " -> " << bucket_name_ << "/" << object_name_;
  return true;
}

void PutObjectCopyCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    Status s = store_->Lock();
    if (s.ok()) {
      // Get src_object_ meta
      s = store_->GetObject(user_name_, src_bucket_name_, src_object_name_,
                            &src_object_);
      if (s.ok()) {
        http_ret_code_ = 200;

        // Initial new_object_
        new_object_.bucket_name = bucket_name_;
        new_object_.object_name = object_name_;
        new_object_.etag = src_object_.etag;
        new_object_.size = src_object_.size;
        new_object_.owner = user_name_;
        new_object_.last_modified = slash::NowMicros();
        new_object_.storage_class = 0; // Unused
        new_object_.acl = "FULL_CONTROL";
        new_object_.upload_id = src_object_.upload_id;
        new_object_.data_block = src_object_.data_block;

        s = store_->AddObject(new_object_, false);
        if (!s.ok()) {
          http_ret_code_ = 500;
          LOG(ERROR) << request_id_ << " " <<
            "PutObjectCopy(DoAndResponse) - AddObject failed: " << s.ToString();
        } else {
          // TODO(gaodq) Add reference to zp
          GenerateRespXml();
        }
      } else {
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
            "PutObjectCopy(DoAndResponse) - GetSrcObject failed: " << s.ToString();
        }
      }

      s = store_->UnLock();
    }
    if (!s.ok()) {
      http_ret_code_ = 500;
      LOG(ERROR) << request_id_ << " " <<
        "PutObjectCopy(DoAndResponse) - Lock or UnLock failed: " << s.ToString();
    }
  }

  g_zgw_monitor->AddApiRequest(kPutObjectCopy, http_ret_code_);
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

void PutObjectCopyCmd::GenerateRespXml() {
  S3XmlDoc doc("CopyObjectResult");
  doc.AppendToRoot(doc.AllocateNode("LastModified",
                                    iso8601_time(new_object_.last_modified)));
  doc.AppendToRoot(doc.AllocateNode("ETag", "\"" + new_object_.etag + "\""));

  doc.ToString(&http_response_xml_);
}

int PutObjectCopyCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
