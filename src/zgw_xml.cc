#include "zgw_store.h"
#include "zgw_xml.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"

namespace xml {

using namespace rapidxml;

const std::string xml_header = "xml version='1.0' encoding='utf-8'";
const std::string xml_ns = "http://s3.amazonaws.com/doc/2006-03-01/";

// Error XML Parser
std::string ErrorXml(ErrorType etype, std::string extra_info) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  // <Error>
  xml_node<> *error = doc.allocate_node(node_element, "Error");
  doc.append_node(error);

  switch(etype) {
    case NoSuchUpload:
      error->append_node(doc.allocate_node(node_element, "Code", "NoSuchUpload"));
      error->append_node(doc.allocate_node(node_element, "Message", "The specified upload does not "
                                           "exist. The upload ID may be invalid, or the upload may "
                                           "have been aborted or completed."));
      error->append_node(doc.allocate_node(node_element, "UploadId", extra_info.c_str()));
      break;
    case BucketAlreadyExists:
      error->append_node(doc.allocate_node(node_element, "Code", "BucketAlreadyExists"));
      error->append_node(doc.allocate_node(node_element, "Message", "The requested bucket "
                                           "name is not available. The bucket namespace is "
                                           "shared by all users of the system. Please select "
                                           "a different name and try again."));
      break;
    case BucketAlreadyOwnedByYou:
      error->append_node(doc.allocate_node(node_element, "Code", "BucketAlreadyOwnedByYou"));
      error->append_node(doc.allocate_node(node_element, "Message", "Your previous request "
                                           "to create the named bucket succeeded and you already own it."));
      break;
    case InvalidAccessKeyId:
      error->append_node(doc.allocate_node(node_element, "Code", "InvalidAccessKeyId"));
      error->append_node(doc.allocate_node(node_element, "Message", "The access key Id "
                                           "you provided does not exist in our records."));
      break;
    case NotImplemented:
      error->append_node(doc.allocate_node(node_element, "Code", "NotImplemented"));
      error->append_node(doc.allocate_node(node_element, "Message", "A header you provided "
                                           "implies functionality that is not implemented."));
      break;
    case SignatureDoesNotMatch:
      error->append_node(doc.allocate_node(node_element, "Code", "SignatureDoesNotMatch"));
      error->append_node(doc.allocate_node(node_element, "Message", "The request signature "
                                           "we calculated does not match the signature you "
                                           "provided. Check your key and signing method."));
      break;
    case NoSuchBucket:
      assert(!extra_info.empty());
      error->append_node(doc.allocate_node(node_element, "Code", "NoSuchBucket"));
      error->append_node(doc.allocate_node(node_element, "Message", "The specified bucket "
                                           "does not exist."));
      error->append_node(doc.allocate_node(node_element, "BucketName", extra_info.c_str()));
      break;
    case NoSuchKey:
      assert(!extra_info.empty());
      error->append_node(doc.allocate_node(node_element, "Code", "NoSuchKey"));
      error->append_node(doc.allocate_node(node_element, "Message", "The specified key "
                                           "does not exist."));
      error->append_node(doc.allocate_node(node_element, "Key", extra_info.c_str()));
      break;
    case BucketNotEmpty:
      assert(!extra_info.empty());
      error->append_node(doc.allocate_node(node_element, "Code", "BucketNotEmpty"));
      error->append_node(doc.allocate_node(node_element, "Message", "The bucket you tried "
                                           "to delete is not empty"));
      error->append_node(doc.allocate_node(node_element, "BucketName", extra_info.c_str()));
      break;
    default:
      break;
  }

  //  <RequestId> TODO gaodq
  error->append_node(doc.allocate_node(node_element, "RequestId",
                                       "tx00000000000000000113c-0058a43a07-7deaf-sh-bt-1"));
  //  <HostId> TODO gaodq
  error->append_node(doc.allocate_node(node_element, "HostId" "7deaf-sh-bt-1-sh"));

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml; 
}

// ListAllBuckets XML Parser
std::string ListBucketXml(const libzgw::ZgwUserInfo &info,
                          const std::vector<libzgw::ZgwBucket> &buckets) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_node<> *rnode = doc.allocate_node(node_element, "ListAllMyBucketsResult");
  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  // <Owner>
  xml_node<> *owner = doc.allocate_node(node_element, "Owner");
  owner->append_node(doc.allocate_node(node_element, "ID",
                                       info.user_id.data()));
  owner->append_node(doc.allocate_node(node_element, "DisplayName",
                                       info.disply_name.data()));
  rnode->append_node(owner);

  // <Buckets>
  xml_node<> *buckets_node = doc.allocate_node(node_element, "Buckets");
  rnode->append_node(buckets_node);

  // Saver for xml parser
  std::vector<std::string> cdates;

  for (auto &bucket : buckets) {
    // <Bucket>
    xml_node<> *bucket_node = doc.allocate_node(node_element, "Bucket");
    bucket_node->append_node(doc.allocate_node(node_element, "Name",
                                               bucket.name().data()));
    cdates.push_back(iso8601_time(bucket.ctime().tv_sec, bucket.ctime().tv_usec));
    bucket_node->append_node(doc.allocate_node(node_element, "CreationDate",
                                               cdates.back().data()));
    buckets_node->append_node(bucket_node);
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

// ListObjects XML Parser
std::string ListObjectsXml(const std::vector<libzgw::ZgwObject> &objects,
                           std::map<std::string, std::string> &args) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  // <ListBucketResult>
  xml_node<> *rnode = doc.allocate_node(node_element, "ListBucketResult");
  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  //    <Name>
  rnode->append_node(doc.allocate_node(node_element, "Name", args["Name"].c_str()));
  //    <Prefix>
  rnode->append_node(doc.allocate_node(node_element, "Prefix", args["Prefix"].c_str()));
  //    <Marker>
  rnode->append_node(doc.allocate_node(node_element, "Marker", args["Marker"].c_str()));
  //    <MaxKeys>
  rnode->append_node(doc.allocate_node(node_element, "MaxKeys", args["MaxKeys"].c_str()));
  //    <IsTruncated>
  rnode->append_node(doc.allocate_node(node_element, "IsTruncated", args["IsTruncated"].c_str()));

  // Saver for xml parser
  std::vector<std::string> cdates;
  std::vector<std::string> sizes;
  for (auto &object : objects) {
    const libzgw::ZgwObjectInfo &info = object.info();
    const libzgw::ZgwUserInfo &user = info.user;
    //  <Contents>
    xml_node<> *content =
      doc.allocate_node(node_element, "Contents");
    rnode->append_node(content);
    //    <Key>
    content->append_node(doc.allocate_node(node_element, "Key",
                                           object.name().data()));
    //    <LastModified>
    cdates.push_back(iso8601_time(info.mtime.tv_sec, info.mtime.tv_usec));
    content->append_node(doc.allocate_node(node_element, "LastModified", 
                                           cdates.back().data()));
    //    <ETag>
    content->append_node(doc.allocate_node(node_element, "ETag",
                                           info.etag.data()));
    //    <Size>
    sizes.push_back(std::to_string(info.size));
    content->append_node(doc.allocate_node(node_element, "Size",
                                           sizes.back().data()));
    //    <StorageClass>
    content->append_node(doc.allocate_node(node_element, "StorageClass",
                                           "STANDARD"));
    //    <Owner>
    xml_node<> *owner =
      doc.allocate_node(node_element, "Owner");
    content->append_node(owner);
    //      <ID>
    owner->append_node(doc.allocate_node(node_element, "ID",
                                         user.user_id.data()));
    //      <DisplayName>
    owner->append_node(doc.allocate_node(node_element, "DisplayName",
                                         user.disply_name.data()));
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

// MultiPartUpload XML Parser
std::string InitiateMultipartUploadResultXml(std::string &bucket_name, std::string &key,
                                             std::string &upload_id) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());

  xml_node<> *rnode = doc.allocate_node(node_element, "InitiateMultipartUploadResult");
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  rnode->append_node(doc.allocate_node(node_element, "Bucket", bucket_name.c_str()));
  rnode->append_node(doc.allocate_node(node_element, "Key", key.c_str()));
  rnode->append_node(doc.allocate_node(node_element, "UploadId", upload_id.c_str()));

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

std::string ListMultipartUploadsResultXml(const std::vector<libzgw::ZgwObject> &objects,
                                          std::map<std::string, std::string> &args) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());

  xml_node<> *rnode = doc.allocate_node(node_element, "ListMultipartUploadsResult");
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  rnode->append_node(doc.allocate_node(node_element, "Bucket", args["Bucket"].c_str()));
  rnode->append_node(doc.allocate_node(node_element, "NextKeyMarker", args["NextKeyMarker"].c_str()));
  rnode->append_node(doc.allocate_node(node_element, "NextUploadIdMarker", args["NextUploadIdMarker"].c_str()));
  rnode->append_node(doc.allocate_node(node_element, "MaxUploads", args["MaxUploads"].c_str()));
  rnode->append_node(doc.allocate_node(node_element, "IsTruncated", args["IsTruncated"].c_str()));

  std::vector<std::string> cdates;
  std::vector<std::string> keys;
  for (auto &object : objects) {
    const libzgw::ZgwObjectInfo &info = object.info();
    const libzgw::ZgwUserInfo &user = info.user;
    std::string name = object.name();
    keys.push_back(name.substr(2, name.size() - 32 - 2));
    xml_node<> *upload = doc.allocate_node(node_element, "Upload");
    upload->append_node(doc.allocate_node(node_element, "Key", keys.back().c_str()));
    upload->append_node(doc.allocate_node(node_element, "UploadId", object.upload_id().c_str()));

    xml_node<> *id = doc.allocate_node(node_element, "ID", user.user_id.data());
    xml_node<> *disply_name = doc.allocate_node(node_element, "DisplayName", user.disply_name.data());
    xml_node<> *id1 = doc.allocate_node(node_element, "ID", user.user_id.data());
    xml_node<> *disply_name1 = doc.allocate_node(node_element, "DisplayName", user.disply_name.data());
    xml_node<> *initiator = doc.allocate_node(node_element, "Initiator");
    xml_node<> *owner = doc.allocate_node(node_element, "Owner");
    initiator->append_node(id);
    initiator->append_node(disply_name);
    owner->append_node(id1);
    owner->append_node(disply_name1);
    upload->append_node(initiator);
    upload->append_node(owner);

    upload->append_node(doc.allocate_node(node_element, "StorageClass", "STANDARD"));
    cdates.push_back(iso8601_time(info.mtime.tv_sec, info.mtime.tv_usec));
    upload->append_node(doc.allocate_node(node_element, "Initiated", cdates.back().c_str()));
    rnode->append_node(upload);
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

std::string iso8601_time(time_t sec, suseconds_t usec) {
  int milli = usec / 1000;
  char buf[128] = {0};
  strftime(buf, sizeof buf, "%FT%T", gmtime(&sec));
  sprintf(buf, "%s.%03dZ", buf, milli);
  return std::string(buf);
}

}  // namespace xml
