#include "src/s3_cmds/zgw_s3_xml.h"

#include <exception>
#include <memory>
#include <vector>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidxml/rapidxml_utils.hpp"

using namespace rapidxml;

struct S3XmlNode::Rep {
  Rep(const std::string& name, const std::string& value, S3XmlDoc* doc)
      : name_(name),
        value_(value),
        doc_(doc),
        rep_node_(nullptr) {
  }
  std::string name_;
  std::string value_;
  S3XmlDoc* doc_;
  xml_node<>* rep_node_;
};

S3XmlNode::S3XmlNode() {
  rep_ = new Rep("", "", nullptr);
}

S3XmlNode::S3XmlNode(const std::string& name, const std::string& value, S3XmlDoc* doc) {
  rep_ = new Rep(name, value, doc);
}

S3XmlNode::~S3XmlNode() {
  delete rep_;
}

void S3XmlNode::AppendNode(const S3XmlNode* new_node) {
  assert(rep_->rep_node_);
  rep_->rep_node_->append_node(new_node->rep_->rep_node_);
}

void S3XmlNode::AppendNode(const std::string& name, const std::string& value) {
  assert(rep_->rep_node_);
  S3XmlNode* new_node = rep_->doc_->AllocateNode(name, value);
  rep_->rep_node_->append_node(new_node->rep_->rep_node_);
}

bool S3XmlNode::FindFirstNode(const std::string& name, S3XmlNode* first_node) {
  assert(rep_->rep_node_);
  xml_node<>* node = rep_->rep_node_->
    first_node(name.c_str(), name.size(), false);
  if (node == nullptr) {
    return false;
  }
  first_node->rep_->name_ = name;
  first_node->rep_->value_ = node->value();
  first_node->rep_->rep_node_ = node;
  return true;
}

bool S3XmlNode::NextSibling() {
  assert(rep_->rep_node_);
  assert(!rep_->name_.empty());
  xml_node<>* node = rep_->rep_node_->
    next_sibling(rep_->name_.c_str(), rep_->name_.size(), false);
  if (node == nullptr) {
    return false;
  }
  rep_->value_ = node->value();
  rep_->rep_node_ = node;
  return true;
}

std::string S3XmlNode::name() {
  return rep_->name_;
}

std::string S3XmlNode::value() {
  return rep_->value_;
}

static const std::string xml_header = "xml version='1.0' encoding='utf-8'";
static const std::string xml_ns = "http://s3.amazonaws.com/doc/2006-03-01/";

struct S3XmlDoc::Rep {
  Rep(const std::string& root_node, const std::string& value)
      : rnode_str_(root_node),
        rnode_value_(value) {
    xml_node<> *rot =
      doc_.allocate_node(node_pi, doc_.allocate_string(xml_header.c_str()));
    doc_.append_node(rot);

    rnode_ = doc_.allocate_node(node_element, rnode_str_.c_str(),
                                rnode_value_.c_str());
    xml_attribute<> *attr = doc_.allocate_attribute("xmlns", xml_ns.c_str());
    rnode_->append_attribute(attr);
    doc_.append_node(rnode_);
  }

  std::string rnode_str_;
  std::string rnode_value_;
  xml_node<>* rnode_;
  xml_document<> doc_;
  std::vector<std::unique_ptr<S3XmlNode>> sub_nodes_;
};

S3XmlDoc::S3XmlDoc(const std::string& root_node, const std::string& value) {
  rep_ = new Rep(root_node, value);
}

S3XmlDoc::S3XmlDoc() {
  rep_ = new Rep("", "");
}

S3XmlDoc::~S3XmlDoc() {
  delete rep_;
}

S3XmlNode* S3XmlDoc::AllocateNode(const std::string& name, const std::string& value) {
  std::unique_ptr<S3XmlNode> new_node(new S3XmlNode(name, value, this));
  xml_node<>* node = rep_->doc_.allocate_node(node_element,
                                              new_node->rep_->name_.c_str(),
                                              new_node->rep_->value_.c_str());
  new_node->rep_->rep_node_ = node;
  rep_->sub_nodes_.push_back(std::move(new_node));
  return rep_->sub_nodes_.back().get();
}

void S3XmlDoc::AppendToRoot(const std::string& name, const std::string& value) {
  S3XmlNode* new_node = AllocateNode(name, value);
  rep_->rnode_->append_node(new_node->rep_->rep_node_);
}

void S3XmlDoc::AppendToRoot(const S3XmlNode* node) {
  rep_->rnode_->append_node(node->rep_->rep_node_);
}

bool S3XmlDoc::ParseFromString(const std::string& xml_str) {
  try {
    rep_->doc_.parse<0>(const_cast<char *>(xml_str.c_str()));
  } catch (std::exception e) {
    return false;
  }
  return true;
}

bool S3XmlDoc::FindFirstNode(const std::string& name, S3XmlNode* first_node) {
  xml_node<>* node = rep_->doc_.first_node(name.c_str(), name.size(), false);
  if (node == nullptr) {
    return false;
  }
  first_node->rep_->name_ = name;
  first_node->rep_->value_ = node->value();
  first_node->rep_->rep_node_ = node;
  return true;
}

void S3XmlDoc::ToString(std::string* res_xml) {
  res_xml->clear();
  print(std::back_inserter(*res_xml), rep_->doc_, 0);
}
