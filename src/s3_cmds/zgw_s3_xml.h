#ifndef ZGW_S3_XMLH
#define ZGW_S3_XMLH

#include <string>

class S3XmlDoc;

class S3XmlNode {
 public:
  void AppendNode(const S3XmlNode* node);
  void AppendNode(const std::string& name, const std::string& value = "");
  bool FindFirstNode(const std::string& name, S3XmlNode* first_node);
  bool NextSibling();
  std::string name();
  std::string value();

  S3XmlNode();
  ~S3XmlNode();
 private:
  friend class S3XmlDoc;

  S3XmlNode(const std::string& name, const std::string& value, S3XmlDoc* doc);

  struct Rep;
  Rep* rep_;
};

class S3XmlDoc {
 public:
  S3XmlDoc(const std::string& root_node, const std::string& value = "");
  S3XmlDoc();
  ~S3XmlDoc();

  S3XmlNode* AllocateNode(const std::string& name, const std::string& value = "");
  void AppendToRoot(const S3XmlNode* node);
  void AppendToRoot(const std::string& name, const std::string& value = "");
  bool ParseFromString(const std::string& xml_str);
  bool FindFirstNode(const std::string& name, S3XmlNode* first_node);
  void ToString(std::string* res_xml);

 private:
  struct Rep;
  Rep* rep_;
};

#endif
