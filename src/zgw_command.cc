#include "src/zgw_command.h"

#include <map>

#include "src/zgw_s3_object.h"

std::map<S3Commands, S3Cmd*> g_cmd_table;

void InitCmdTable() {
  // g_cmd_table.insert(std::make_pair(kListAllBuckets, new ListAllBucketsCmd()));
  // g_cmd_table.insert(std::make_pair(kDeleteBucket, new DeleteBucketCmd()));
  // g_cmd_table.insert(std::make_pair(kListObjects, new ListObjectsCmd()));
  // g_cmd_table.insert(std::make_pair(kGetBucketLocation, new GetBucketLocation()));
  // g_cmd_table.insert(std::make_pair(kHeadBucket, new HeadBucketCmd()));
  // g_cmd_table.insert(std::make_pair(kListMultiPartUpload, new ListMultiPartUploadCmd()));
  // g_cmd_table.insert(std::make_pair(kPutBucket, new PutBucketCmd()));
  // g_cmd_table.insert(std::make_pair(kDeleteObject, new DeleteObjectCmd()));
  // g_cmd_table.insert(std::make_pair(kDeleteMultiObjects, new DeleteMultiObjectsCmd()));
  g_cmd_table.insert(std::make_pair(kGetObject, new GetObjectCmd()));
  // g_cmd_table.insert(std::make_pair(kHeadObject, new HeadObjectCmd()));
  // g_cmd_table.insert(std::make_pair(kPostObject, new PostObjectCmd()));
  g_cmd_table.insert(std::make_pair(kPutObject, new PutObjectCmd()));
  // g_cmd_table.insert(std::make_pair(kPutObjectCopy, new PutObjectCopyCmd()));
  // g_cmd_table.insert(std::make_pair(kInitMultipartUpload, new InitMultipartUploadCmd()));
  // g_cmd_table.insert(std::make_pair(kUploadPart, new UploadPartCmd()));
  // g_cmd_table.insert(std::make_pair(kUploadPartCopy, new UploadPartCopyCmd()));
  // g_cmd_table.insert(std::make_pair(kCompleteMultiUpload, new CompleteMultiUploadCmd()));
  // g_cmd_table.insert(std::make_pair(kAbortMultiUpload, new AbortMultiUploadCmd()));
  // g_cmd_table.insert(std::make_pair(kListParts, new ListPartsCmd()));
  // g_cmd_table.insert(std::make_pair(kUnImplement, new UnImplementCmd()));
}

void DestroyCmdTable() {
  for (auto& cmd : g_cmd_table) {
    delete cmd.second;
  }
}

void S3Cmd::Clear() {
  bucket_name_.clear();
  object_name_.clear();
  req_headers_.clear();
  query_params_.clear();
  http_request_xml_.clear();

  http_ret_code_ = 200;
  http_response_xml_.clear();
}
