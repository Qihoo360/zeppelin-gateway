#include "src/zgw_s3_rest.h"

#include <glog/logging.h>
#include "src/zgwstore/zgw_store.h"
#include "src/s3_cmds/zgw_s3_command.h"
#include "src/zgw_utils.h"
#include "src/zgw_server.h"

bool ZgwHttpHandles::ReqHeadersHandle(const pink::HttpRequest* req) {
  // req->Dump();

  cmd_ = SelectS3Cmd(req);
  if (!cmd_->DoInitial()) {
    // Something wrong happend, need reply right now
    return true;
  }

  if (req->headers_.count("expect")) {
    Timer t("Send 100-continue -");
    LOG(INFO) << "Expect 100-continue";
    need_100_continue_ = true;
    // Need reply right now
    return true;
  } else {
    need_100_continue_ = false;
  }

  // Needn't reply right now
  return false;
}

void ZgwHttpHandles::ReqBodyPartHandle(const char* data, size_t data_size) {
  cmd_->DoReceiveBody(data, data_size);
}

void ZgwHttpHandles::RespHeaderHandle(pink::HttpResponse* resp) {
  if (need_100_continue_) {
    resp->SetStatusCode(100);
    resp->SetContentLength(0);
    need_100_continue_ = false;
    return;
  }

  cmd_->DoAndResponse(resp);
}

int ZgwHttpHandles::RespBodyPartHandle(char* buf, size_t max_size) {
  if (need_100_continue_) {
    return 0;
  }

  return cmd_->DoResponseBody(buf, max_size);
}

S3Cmd* ZgwHttpHandles::SelectS3Cmd(const pink::HttpRequest* req) {
  std::string bucket_name, object_name;
  SplitBySecondSlash(req->path_, &bucket_name, &object_name);

  S3Commands cmd = kUnImplement;
  if (req->method_ == "GET" &&
      bucket_name.empty() &&
      object_name.empty()) {
    cmd = kListAllBuckets;
  } else if (!bucket_name.empty() && object_name.empty()) {
    // Bucket operation
    if (req->method_ == "GET") {
      if (req->query_params_.count("uploads")) {
        cmd = kListMultiPartUpload;
      } else if (req->query_params_.count("location")) {
        cmd = kGetBucketLocation;
      } else {
        cmd = kListObjects;
      }
    } else if (req->method_ == "PUT") {
      cmd = kPutBucket;
    } else if (req->method_ == "DELETE") {
      cmd = kDeleteBucket;
    } else if (req->method_ == "HEAD") {
      cmd = kHeadBucket;
    } else if (req->method_ == "POST") {
      if (req->query_params_.count("delete")) {
        cmd = kDeleteMultiObjects;
      }
    }
  } else if (!bucket_name.empty() && !object_name.empty()) {
    // Object operation
    if (req->method_ == "GET") {
      if (req->query_params_.count("uploadId")) {
        cmd = kListParts;
      } else {
        cmd = kGetObject;
      }
    } else if (req->method_ == "PUT") {
      if (req->query_params_.count("partNumber") &&
          req->query_params_.count("uploadId")) {
        if (req->headers_.count("x-amz-copy-source")) {
          cmd = kUploadPartCopy;
        } else {
          cmd = kUploadPart;
        }
      } else {
        if (req->headers_.count("x-amz-copy-source")) {
          cmd = kPutObjectCopy;
        } else {
          cmd = kPutObject;
        }
      }
    } else if (req->method_ == "DELETE") {
      if (req->query_params_.count("uploadId")) {
        cmd = kAbortMultiUpload;
      } else {
        cmd = kDeleteObject;
      }
    } else if (req->method_ == "HEAD") {
      cmd = kHeadObject;
    } else if (req->method_ == "POST") {
      if (req->query_params_.count("uploads")) {
        cmd = kInitMultipartUpload;
      } else if (req->query_params_.count("uploadId")) {
        cmd = kCompleteMultiUpload;
      }
    }
  }

  zgwstore::ZgwStore* store =
    static_cast<zgwstore::ZgwStore*>(thread_ptr_->get_private());

  assert(cmd_table_->count(cmd) > 0);
  S3Cmd* cmd_ptr = cmd_table_->at(cmd);
  cmd_ptr->Clear();
  cmd_ptr->SetBucketName(bucket_name);
  cmd_ptr->SetObjectName(object_name);
  cmd_ptr->SetReqHeaders(req->headers_);
  cmd_ptr->SetQueryParams(req->query_params_);
  cmd_ptr->SetStorePtr(store);
  cmd_ptr->InitS3Auth(req);

  return cmd_ptr;
}
