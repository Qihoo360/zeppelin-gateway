#include "src/s3_cmds/zgw_s3_object.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgw_utils.h"

bool InitMultipartUploadCmd::DoInitial(pink::HTTPResponse* resp) {
  upload_id_ = md5(bucket_name_ + object_name_ +
                   std::to_string(slash::NowMicros()));
  DLOG(INFO) << "InitialMultiUpload(DoInitial) - " <<
    bucket_name_ << "/" << object_name_ << "-uploadId: " << upload_id_;

  if (!TryAuth()) {
    return false;
  }

  zgwstore::Bucket bucket;
  Status s = store_->GetBucket(user_name_, bucket_name_, &bucket);
  if (!s.ok()) {
    if (s.ToString().find("Bucket Doesn't Belong To This User") ||
        s.ToString().find("Bucket Not Found")) {
      http_ret_code_ = 404;
      GenerateErrorXml(kNoSuchBucket, bucket_name_);
    } else {
      http_ret_code_ = 500;
      LOG(ERROR) << "InitialMultiUpload(DoInitial) - GetBucket Error " <<
        s.ToString();
    }
    return false;
  }

  return true;
}

void InitMultipartUploadCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    // Initial new virtual bucket as multipart object meta
    zgwstore::Bucket virtual_bucket;
    virtual_bucket.bucket_name = "__TMPB" + upload_id_ +
      bucket_name_ + "|" + object_name_;
    virtual_bucket.create_time = slash::NowMicros();
    virtual_bucket.owner = user_name_;
    virtual_bucket.acl = "FULL_CONTROL"; // TODO (gaodq) acl
    virtual_bucket.location = "CN"; // default
    virtual_bucket.volumn = 0;
    virtual_bucket.uploading_volumn = 0;

    Status s = store_->AddBucket(virtual_bucket);
    if (!s.ok()) {
      // Duplicate bucket must not exist
      http_ret_code_ = 500;
      LOG(ERROR) << "InitialMultiUpload(DoAndResponse) - Error Add virtual_bucket: "
        << virtual_bucket.bucket_name;
    }
    GenerateRespXml();
    DLOG(INFO) << "InitialMultiUpload(DoAndResponse) - Add virtual_bucket success";
  }

  DLOG(INFO) << "InitialMultiUpload(DoAndResponse) - http code " << http_ret_code_;
  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

void InitMultipartUploadCmd::GenerateRespXml() {
  S3XmlDoc doc("InitiateMultipartUploadResult");
  doc.AppendToRoot(doc.AllocateNode("Bucket", bucket_name_));
  doc.AppendToRoot(doc.AllocateNode("Key", object_name_));
  doc.AppendToRoot(doc.AllocateNode("UploadId", upload_id_));
  doc.ToString(&http_response_xml_);
}

int InitMultipartUploadCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
