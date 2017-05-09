#ifndef ZGW_XMLBUILDER_H
#define ZGW_XMLBUILDER_H

#include <string>

class S3XmlNode {
 public:
  void AppendNode(const S3XmlNode* node);
  bool IsValid();
  S3XmlNode FindFirstNode(const std::string& name);
  S3XmlNode NextSibling();
  S3XmlNode NextSibling(const std::string& name);

  ~S3XmlNode();
 private:
  friend class S3XmlDoc;

  S3XmlNode(const std::string& name, const std::string& value = "");

  struct Rep;
  Rep* rep_;
};

class S3XmlDoc {
 public:
  S3XmlDoc(const std::string& root_node);
  ~S3XmlDoc();

  S3XmlNode* AllocateNode(const std::string& name, const std::string& value = "");
  void AppendToRoot(const S3XmlNode* node);
  bool ParseFromString(const std::string& xml_str);
  S3XmlNode FindFirstNode(const std::string& name);
  std::string ToString();

 private:
  struct Rep;
  Rep* rep_;
};

#endif
