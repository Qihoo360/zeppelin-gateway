#ifndef ZGW_XMLBUILDER_H
#define ZGW_XMLBUILDER_H

#include <memory>
#include <exception>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidxml/rapidxml_utils.hpp"

using namespace rapidxml;

class S3XmlNode {
 public:
  void AppendNode(const S3XmlNode& node) {
    this->rep_->append_node(node.rep_);
  }

  bool IsValid() {
    return rep_ != nullptr;
  }

  S3XmlNode FindFirstNode(const std::string& name) {
    S3XmlNode s3_node;
    xml_node<>* node = rep_->first_node(name.c_str(), name.size(), false);
    s3_node.name_ = name;
    s3_node.value_ = node->value();
    s3_node.rep_ = node;
    return s3_node;
  }

  S3XmlNode NextSibling() {
    return NextSibling(name_);
  }

  S3XmlNode NextSibling(const std::string& name) {
    S3XmlNode s3_node;
    xml_node<>* nexts = rep_->next_sibling(name.c_str(), name.size(), false);
    s3_node.name_ = name;
    s3_node.value_ = value;
    s3_node.rep_ = nexts;
    return s3_node;
  }

 private:
  friend class S3XmlDoc;

  S3XmlNode(const std::string& name, const std::string& value = "")
      : name_(name),
        value_(value),
        rep_(nullptr) {
  }

  std::string name_;
  std::string value_;
  xml_node<>* rep_;
}

class S3XmlDoc {
 public:
  static std::string xml_header = "xml version='1.0' encoding='utf-8'";
  static std::string xml_ns = "http://s3.amazonaws.com/doc/2006-03-01/";

  S3XmlDoc(std::string& root_node) {
    xml_node<> *rot =
      rep_.allocate_node(node_pi, rep_.allocate_string(xml_header.c_str()));
    rep_.append_node(rot);

    rnode_ = rep_.allocate_node(node_element, root_node.c_str());
    xml_attribute<> *attr = rep_.allocate_attribute("xmlns", xml_ns.c_str());
    rnode->append_attribute(attr);
    rep_.append_node(rnode_);
  }

  S3XmlNode AllocateNode(const std::string& name, const std::string& value = "") {
    S3XmlNode new_node(name, value);
    xml_node<>* node = rep_.allocate_node(node_element,
                                          new_node.name_.c_str(),
                                          new_node.value_.c_str());
    new_node.rep_ = node;
  }

  void AppendToRoot(const S3XmlNode& node) {
    rnode_->.append_node(node.rep_);
  }

  bool ParseFromString(const std::string& xml_str) {
    try {
      rep_.parse<0>(const_cast<char *>(xml_str.c_str()));
    } catch (std::exception e) {
      return false;
    }
    return true;
  }

  S3XmlNode FindFirstNode(const std::string& name) {
    S3XmlNode s3_node;
    xml_node<>* node = rep_.first_node(name.c_str(), name.size(), false);
    s3_node.name_ = name;
    s3_node.value_ = node->value();
    s3_node.rep_ = node;
    return s3_node;
  }

  std::string ToString() {
    print(std::back_inserter(res_xml_), rep_, 0);
    return res_xml_; 
  }

 private:
  xml_node<>* rnode_;
  xml_document<> rep_;
  std::string res_xml_;
};

// enum ErrorType {
//   InvalidAccessKeyId,
//   SignatureDoesNotMatch,
//   NoSuchBucket,
//   NoSuchKey,
//   BucketNotEmpty,
//   NoSuchUpload,
//   NotImplemented,
//   BucketAlreadyOwnedByYou,
//   BucketAlreadyExists,
//   MalformedXML,
//   InvalidPart,
//   InvalidPartOrder,
//   MethodNotAllowed,
//   InvalidArgument,
//   InvalidRange,
//   AccessDenied,
// };
// 
// extern std::string ErrorXml(ErrorType etype, const std::string& extra_info = "");
// extern std::string ListBucketXml(const libzgw::ZgwUserInfo &info,
//                                  const std::vector<libzgw::ZgwBucket> &buckets);
// extern std::string GetBucketLocationXml();
// extern std::string ListObjectsXml(const std::vector<libzgw::ZgwObject> &objects,
//                                   const std::map<std::string, std::string> &args,
//                                   const std::set<std::string>& commonprefixes);
// extern std::string InitiateMultipartUploadResultXml(const std::string &bucket_name, const std::string &key,
//                                                     const std::string &upload_id);
// extern std::string ListMultipartUploadsResultXml(const std::vector<libzgw::ZgwObject> &objects,
//                                                  const std::map<std::string, std::string> &args,
//                                                  const std::set<std::string>& commonprefixes);
// extern std::string ListPartsResultXml(const std::vector<std::pair<int, libzgw::ZgwObject>> &objects,
//                                       const libzgw::ZgwUserInfo& user_info,
//                                       const std::map<std::string, std::string> &args);
// extern std::string CompleteMultipartUploadResultXml(const std::string& bucket_name,
//                                                     const std::string& object_name,
//                                                     const std::string& final_etag);
// extern std::string CopyObjectResultXml(timeval now, const std::string& etag);
// extern std::string DeleteResultXml(const std::vector<std::string>& success_keys,
//                                    const std::map<std::string, std::string>& error_keys);
// extern bool ParseCompleteMultipartUploadXml(const std::string& xml,
//                                             std::vector<std::pair<int, std::string>> *parts);
// extern bool ParseDelMultiObjectXml(const std::string& xml, std::vector<std::string> *keys);

#endif
