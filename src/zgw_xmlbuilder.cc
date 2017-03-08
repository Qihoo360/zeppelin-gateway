#include "zgw_store.h"
#include "zgw_xmlbuilder.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"

using namespace rapidxml;

const std::string xml_header = "xml version='1.0' encoding='utf-8'";
const std::string xml_ns = "http://s3.amazonaws.com/doc/2006-03-01/";

// Error XML Parser
std::string XmlParser(ErrorType etype, std::string extra_info) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  // <Error>
  xml_node<> *error = doc.allocate_node(node_element, "Error");
  doc.append_node(error);

  switch(etype) {
    case InvalidAccessKeyId:
      error->append_node(doc.allocate_node(node_element, "Code", "InvalidAccessKeyId"));
      error->append_node(doc.allocate_node(node_element, "Message", "The access key Id "
                                           "you provided does not exist in our records."));
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
std::string XmlParser(const libzgw::ZgwUserInfo &info,
                      const std::vector<libzgw::ZgwBucket> &buckets) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_node<> *rnode = doc.allocate_node(node_element, "ListAllMyBucketsResult");
  xml_attribute<> *attr =
    doc.allocate_attribute("xmlns", xml_ns.c_str());
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
std::string XmlParser(const std::vector<libzgw::ZgwObject> &objects,
                      std::map<std::string, std::string> &args) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  // <ListBucketResult>
  xml_node<> *rnode =
    doc.allocate_node(node_element, "ListBucketResult");
  xml_attribute<> *attr =
    doc.allocate_attribute("xmlns", xml_ns.c_str());
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
      doc.allocate_node(node_element, "Contents", "");
    rnode->append_node(content);
    //    <Key>
    content->append_node(doc.allocate_node(node_element, "Key",
                                           object.name().data()));
    //    <LastModified>
    cdates.push_back(iso8601_time(info.mtime));
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

std::string iso8601_time(time_t sec, suseconds_t usec) {
  int milli = usec / 1000;
  char buf[128] = {0};
  strftime(buf, sizeof buf, "%FT%T", localtime(&sec));
  sprintf(buf, "%s.%03dZ", buf, milli);
  return std::string(buf);
}
