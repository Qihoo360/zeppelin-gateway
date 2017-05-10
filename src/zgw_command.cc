#include "src/zgw_command.h"

#include <map>

#include "src/zgw_s3_object.h"
#include "src/zgw_s3_bucket.h"
#include "src/zgw_xml.h"

std::map<S3Commands, S3Cmd*> g_cmd_table;

void InitCmdTable() {
  g_cmd_table.insert(std::make_pair(kListAllBuckets, new ListAllBucketsCmd()));
  // g_cmd_table.insert(std::make_pair(kDeleteBucket, new DeleteBucketCmd()));
  // g_cmd_table.insert(std::make_pair(kListObjects, new ListObjectsCmd()));
  // g_cmd_table.insert(std::make_pair(kGetBucketLocation, new GetBucketLocation()));
  // g_cmd_table.insert(std::make_pair(kHeadBucket, new HeadBucketCmd()));
  // g_cmd_table.insert(std::make_pair(kListMultiPartUpload, new ListMultiPartUploadCmd()));
  g_cmd_table.insert(std::make_pair(kPutBucket, new PutBucketCmd()));
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

void S3Cmd::GenerateErrorXml(S3ErrorType type, const std::string& message) {
  http_response_xml_.clear();
  S3XmlDoc doc("Error");
  switch(type) {
    case kAccessDenied:
      doc.AppendToRoot(doc.AllocateNode("Code", "AccessDenied"));
      doc.AppendToRoot(doc.AllocateNode("Message", "Access Denied"));
      break;
    case kInvalidRange:
      doc.AppendToRoot(doc.AllocateNode("Code", "InvalidRange"));
      doc.AppendToRoot(doc.AllocateNode("BucketName", message));
      break;
    case kInvalidArgument:
      doc.AppendToRoot(doc.AllocateNode("Code", "InvalidArgument"));
      doc.AppendToRoot(doc.AllocateNode("Message", "Copy Source must mention the "
                                        "source bucket and key: "
                                        "sourcebucket/sourcekey"));
      doc.AppendToRoot(doc.AllocateNode("ArgumentName", message));
      break;
    case kMethodNotAllowed:
      doc.AppendToRoot(doc.AllocateNode("Code", "MethodNotAllowed"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The specified method is not "
                                        "allowed against this resource."));
      break;
    case kInvalidPartOrder:
      doc.AppendToRoot(doc.AllocateNode("Code", "InvalidPartOrder"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The list of parts was not in "
                                        "ascending order. The parts list must be "
                                        "specified in order by part number."));
      break;
    case kInvalidPart:
      doc.AppendToRoot(doc.AllocateNode("Code", "InvalidPart"));
      doc.AppendToRoot(doc.AllocateNode("Message", "One or more of the specified "
                                        "parts could not be found. The part might "
                                        "not have been uploaded, or the specified "
                                        "entity tag might not have matched the "
                                        "part's entity tag."));
      break;
    case kMalformedXML:
      doc.AppendToRoot(doc.AllocateNode("Code", "MalformedXML"));
      doc.AppendToRoot(doc.AllocateNode("Message", "This happens when the user "
                                        "sends malformed xml (xml that doesn't "
                                                             "conform to the "
                                                             "published xsd) for "
                                        "the configuration. The error message is, "
                                        "\"The XML you provided was not well-formed "
                                        "or did not validate against our published "
                                        "schema.\""));
      break;
    case kNoSuchUpload:
      doc.AppendToRoot(doc.AllocateNode("Code", "NoSuchUpload"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The specified upload does not "
                                        "exist. The upload ID may be invalid, or "
                                        "the upload may have been aborted or "
                                        "completed."));
      doc.AppendToRoot(doc.AllocateNode("UploadId", message));
      break;
    case kBucketAlreadyExists:
      doc.AppendToRoot(doc.AllocateNode("Code", "BucketAlreadyExists"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The requested bucket name is "
                                        "not available. The bucket namespace is "
                                        "shared by all users of the system. Please "
                                        "select a different name and try again."));
      break;
    case kBucketAlreadyOwnedByYou:
      doc.AppendToRoot(doc.AllocateNode("Code", "BucketAlreadyOwnedByYou"));
      doc.AppendToRoot(doc.AllocateNode("Message", "Your previous request to "
                                        "create the named bucket succeeded and you "
                                        "already own it."));
      break;
    case kInvalidAccessKeyId:
      doc.AppendToRoot(doc.AllocateNode("Code", "InvalidAccessKeyId"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The access key Id you provided "
                                        "does not exist in our records."));
      break;
    case kNotImplemented:
      doc.AppendToRoot(doc.AllocateNode("Code", "NotImplemented"));
      doc.AppendToRoot(doc.AllocateNode("Message", "A header you provided implies "
                                        "functionality that is not implemented."));
      break;
    case kSignatureDoesNotMatch:
      doc.AppendToRoot(doc.AllocateNode("Code", "SignatureDoesNotMatch"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The request signature we "
                                        "calculated does not match the signature "
                                        "you provided. Check your key and signing "
                                        "method."));
      break;
    case kNoSuchBucket:
      doc.AppendToRoot(doc.AllocateNode("Code", "NoSuchBucket"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The specified bucket does not "
                                        "exist."));
      doc.AppendToRoot(doc.AllocateNode("BucketName", message));
      break;
    case kNoSuchKey:
      doc.AppendToRoot(doc.AllocateNode("Code", "NoSuchKey"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The specified key does not "
                                        "exist."));
      doc.AppendToRoot(doc.AllocateNode("Key", message));
      break;
    case kBucketNotEmpty:
      doc.AppendToRoot(doc.AllocateNode("Code", "BucketNotEmpty"));
      doc.AppendToRoot(doc.AllocateNode("Message", "The bucket you tried to delete "
                                        "is not empty"));
      doc.AppendToRoot(doc.AllocateNode("BucketName", message));
      break;
    default:
      break;
  }
  doc.ToString(&http_response_xml_);
}
