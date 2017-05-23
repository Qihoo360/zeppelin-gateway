#include "src/zgw_s3_rest.h"

#include <glog/logging.h>
#include "slash/include/slash_hash.h"
#include "slash/include/env.h"
#include "src/zgwstore/zgw_store.h"
#include "src/s3_cmds/zgw_s3_command.h"
#include "src/zgw_utils.h"
#include "src/zgw_server.h"

bool ZgwHTTPHandles::HandleRequest(const pink::HTTPRequest* req) {
  // req->Dump();

  cmd_ = SelectS3CmdBy(req);

  if (!cmd_->DoInitial()) {
    // Something wrong happend, need reply right now
    return true;
  }

  // Needn't reply right now
  return false;
}

void ZgwHTTPHandles::HandleBodyData(const char* data, size_t data_size) {
  cmd_->DoReceiveBody(data, data_size);
}

void ZgwHTTPHandles::PrepareResponse(pink::HTTPResponse* resp) {
  cmd_->DoAndResponse(resp);
  resp->SetHeaders("x-amz-request-id", cmd_->request_id());
  resp->SetHeaders("Date", http_nowtime(slash::NowMicros()));
  resp->SetHeaders("Server", "Zeppelin gateway 2.0");
}

int ZgwHTTPHandles::WriteResponseBody(char* buf, size_t max_size) {
  return cmd_->DoResponseBody(buf, max_size);
}

void ZgwHTTPHandles::HandleConnClosed() {
  return cmd_->DoConnClosed();
}

S3Cmd* ZgwHTTPHandles::SelectS3CmdBy(const pink::HTTPRequest* req) {
  std::string bucket_name, object_name;
  SplitBySecondSlash(req->path(), &bucket_name, &object_name);

  S3Commands cmd = kUnImplement;
  if (req->method() == "GET" &&
      bucket_name.empty() &&
      object_name.empty()) {
    cmd = kListAllBuckets;
  } else if (!bucket_name.empty() && object_name.empty()) {
    // Bucket operation
    if (req->method() == "GET") {
      if (req->query_params().count("uploads")) {
        cmd = kListMultiPartUpload;
      } else if (req->query_params().count("location")) {
        cmd = kGetBucketLocation;
      } else {
        cmd = kListObjects;
      }
    } else if (req->method() == "PUT") {
      cmd = kPutBucket;
    } else if (req->method() == "DELETE") {
      cmd = kDeleteBucket;
    } else if (req->method() == "HEAD") {
      if (bucket_name == "_zgwtest") {
        cmd = kZgwTest;
      } else {
        cmd = kHeadBucket;
      }
    } else if (req->method() == "POST") {
      if (req->query_params().count("delete")) {
        cmd = kDeleteMultiObjects;
      }
    }
  } else if (!bucket_name.empty() && !object_name.empty()) {
    // Object operation
    if (req->method() == "GET") {
      if (req->query_params().count("uploadId")) {
        cmd = kListParts;
      } else {
        cmd = kGetObject;
      }
    } else if (req->method()== "PUT") {
      if (req->query_params().count("partNumber") &&
          req->query_params().count("uploadId")) {
        if (req->headers().count("x-amz-copy-source")) {
          if (req->headers().count("x-amz-copy-source-range")) {
            cmd = kUploadPartCopyPartial;
          } else {
            cmd = kUploadPartCopy;
          }
        } else {
          cmd = kUploadPart;
        }
      } else {
        if (req->headers().count("x-amz-copy-source")) {
          cmd = kPutObjectCopy;
        } else {
          cmd = kPutObject;
        }
      }
    } else if (req->method()== "DELETE") {
      if (req->query_params().count("uploadId")) {
        cmd = kAbortMultiUpload;
      } else {
        cmd = kDeleteObject;
      }
    } else if (req->method()== "HEAD") {
      cmd = kHeadObject;
    } else if (req->method()== "POST") {
      if (req->query_params().count("uploads")) {
        cmd = kInitMultipartUpload;
      } else if (req->query_params().count("uploadId")) {
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
  cmd_ptr->SetReqHeaders(req->headers());
  cmd_ptr->SetQueryParams(req->query_params());
  cmd_ptr->SetStorePtr(store);
  cmd_ptr->InitS3Auth(req);

  return cmd_ptr;
}
