#include "zgw_xml.h"

#include <exception>

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"

namespace xml {

using namespace rapidxml;

const std::string xml_header = "xml version='1.0' encoding='utf-8'";
const std::string xml_ns = "http://s3.amazonaws.com/doc/2006-03-01/";

inline std::string ExtraParNum(std::string, std::string);
std::string iso8601_time(time_t, suseconds_t);

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
    case InvalidArgument:
      error->append_node(doc.allocate_node(node_element, "Code", "InvalidArgument"));
      error->append_node(doc.allocate_node(node_element, "Message", "Copy Source must "
                                           "mention the source bucket and key: sourcebucket/sourcekey"));
      error->append_node(doc.allocate_node(node_element, "ArgumentName", extra_info.c_str()));
      break;
    case MethodNotAllowed:
      error->append_node(doc.allocate_node(node_element, "Code", "MethodNotAllowed"));
      error->append_node(doc.allocate_node(node_element, "Message", "The specified method "
                                           "is not allowed against this resource."));
      break;
    case InvalidPartOrder:
      error->append_node(doc.allocate_node(node_element, "Code", "InvalidPartOrder"));
      error->append_node(doc.allocate_node(node_element, "Message", "The list of parts "
                                           "was not in ascending order. The parts list must "
                                           "be specified in order by part number."));
      break;
    case InvalidPart:
      error->append_node(doc.allocate_node(node_element, "Code", "InvalidPart"));
      error->append_node(doc.allocate_node(node_element, "Message", "One or more of "
                                           "the specified parts could not be found. The "
                                           "part might not have been uploaded, or the "
                                           "specified entity tag might not have matched "
                                           "the part's entity tag."));
      break;
    case MalformedXML:
      error->append_node(doc.allocate_node(node_element, "Code", "MalformedXML"));
      error->append_node(doc.allocate_node(node_element, "Message", "This happens "
                                           "when the user sends malformed xml (xml "
                                           "that doesn't conform to the published xsd) "
                                           "for the configuration. The error message is, "
                                           "\"The XML you provided was not well-formed "
                                           "or did not validate against our published schema.\""));
      break;
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

  for (auto &it : args) {
    rnode->append_node(doc.allocate_node(node_element, it.first.c_str(), it.second.c_str()));
  }

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

  for (auto &it : args) {
    rnode->append_node(doc.allocate_node(node_element, it.first.c_str(), it.second.c_str()));
  }

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

std::string ListPartsResultXml(const std::vector<std::pair<int, libzgw::ZgwObject>> &objects,
                               libzgw::ZgwUser *user, std::map<std::string, std::string> &args) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());

  xml_node<> *rnode = doc.allocate_node(node_element, "ListPartsResult");
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  for (auto &it : args) {
    rnode->append_node(doc.allocate_node(node_element, it.first.c_str(), it.second.c_str()));
  }

  const libzgw::ZgwUserInfo &user_info = user->user_info();
  xml_node<> *id = doc.allocate_node(node_element, "ID", user_info.user_id.data());
  xml_node<> *disply_name = doc.allocate_node(node_element, "DisplayName", user_info.disply_name.data());
  xml_node<> *id1 = doc.allocate_node(node_element, "ID", user_info.user_id.data());
  xml_node<> *disply_name1 = doc.allocate_node(node_element, "DisplayName", user_info.disply_name.data());
  xml_node<> *initiator = doc.allocate_node(node_element, "Initiator");
  xml_node<> *owner = doc.allocate_node(node_element, "Owner");
  initiator->append_node(id);
  initiator->append_node(disply_name);
  owner->append_node(id1);
  owner->append_node(disply_name1);
  rnode->append_node(initiator);
  rnode->append_node(owner);

  std::vector<std::string> size;
  std::vector<std::string> part_num;
  std::vector<std::string> last_modified;
  for (auto& it : objects) {
    const libzgw::ZgwObjectInfo &info = it.second.info();
    part_num.push_back(std::to_string(it.first));
    xml_node<> *part = doc.allocate_node(node_element, "Part");
    part->append_node(doc.allocate_node(node_element, "PartNumber", part_num.back().c_str()));
    last_modified.push_back(iso8601_time(info.mtime.tv_sec, info.mtime.tv_usec));
    part->append_node(doc.allocate_node(node_element, "LastModified", last_modified.back().c_str()));
    part->append_node(doc.allocate_node(node_element, "ETag", info.etag.c_str()));
    size.push_back(std::to_string(info.size));
    part->append_node(doc.allocate_node(node_element, "Size", size.back().c_str()));
    rnode->append_node(part);
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

std::string DeleteResultXml(const std::vector<std::string>& success_keys,
                            const std::map<std::string, std::string>& error_keys) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());

  xml_node<> *rnode = doc.allocate_node(node_element, "DeleteResult");
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  for (auto& skey : success_keys) {
    xml_node<> *deleted = doc.allocate_node(node_element, "Deleted");
    rnode->append_node(deleted);
    xml_node<> *key = doc.allocate_node(node_element, "Key", skey.c_str());
    deleted->append_node(key);
  }

  for (auto& it : error_keys) {
    xml_node<> *error = doc.allocate_node(node_element, "Error");
    rnode->append_node(error);
    error->append_node(doc.allocate_node(node_element, "Key", it.first.c_str()));
    error->append_node(doc.allocate_node(node_element, "Code", it.second.c_str()));
  }

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

std::string CompleteMultipartUploadResultXml(const std::string& bucket_name,
                                             const std::string& object_name,
                                             const std::string& final_etag) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());

  xml_node<> *rnode = doc.allocate_node(node_element, "CompleteMultipartUploadResult");
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  rnode->append_node(doc.allocate_node(node_element, "Bucket", bucket_name.c_str()));;
  rnode->append_node(doc.allocate_node(node_element, "Key", object_name.c_str()));;
  rnode->append_node(doc.allocate_node(node_element, "ETag", final_etag.c_str()));;

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

std::string CopyObjectResultXml(timeval now, const std::string& etag) {
  // <Root>
  xml_document<> doc;
  xml_node<> *rot =
    doc.allocate_node(node_pi, doc.allocate_string(xml_header.c_str()));
  doc.append_node(rot);

  xml_attribute<> *attr = doc.allocate_attribute("xmlns", xml_ns.c_str());

  xml_node<> *rnode = doc.allocate_node(node_element, "CompleteMultipartUploadResult");
  rnode->append_attribute(attr);
  doc.append_node(rnode);

  std::string time = iso8601_time(now.tv_sec, now.tv_usec);
  rnode->append_node(doc.allocate_node(node_element, "LastModified", time.c_str()));;
  rnode->append_node(doc.allocate_node(node_element, "ETag", etag.c_str()));;

  std::string res_xml;
  print(std::back_inserter(res_xml), doc, 0);
  return res_xml;
}

bool ParseCompleteMultipartUploadXml(const std::string& xml,
                                     std::vector<std::pair<int, std::string>> *parts) {
  xml_document<> doc;
  try {
    doc.parse<0>(const_cast<char *>(xml.c_str()));
  } catch (std::exception e) {
    return false;
  }

  const char *root_s = "CompleteMultipartUpload";
  const char *part_s = "Part";
  const char *part_num_s = "PartNumber";
  const char *etag_s = "ETag";
  xml_node<> *root_node = doc.first_node(root_s, strlen(root_s), false);
  if (!root_node) {
    return false;
  }
  for (xml_node<> *part = root_node->first_node(part_s, strlen(part_s), false);
       part; part = part->next_sibling(part_s, strlen(part_s), false)) {
    xml_node<> *part_num = part->first_node(part_num_s, strlen(part_num_s), false);
    if (!part_num) {
      return false;
    }
    xml_node<> *etag = part->first_node(etag_s, strlen(etag_s), false);
    if (!etag) {
      return false;
    }
    std::string quote_etag = etag->value();
    if (quote_etag.at(0) != '\"') {
      quote_etag.assign("\"" + quote_etag + "\"");
    }
    parts->push_back(std::make_pair(std::atoi(part_num->value()), quote_etag));
  }
  return true;
}

bool ParseDelMultiObjectXml(const std::string& xml, std::vector<std::string> *keys) {
  xml_document<> doc;
  try {
    doc.parse<0>(const_cast<char *>(xml.c_str()));
  } catch (std::exception e) {
    return false;
  }

  const char *delete_s = "Delete";
  const char *object_s = "Object";
  const char *key_s = "Key";
  xml_node<> *del = doc.first_node(delete_s, strlen(delete_s), false);
  if (!del) {
    return false;
  }

  for (xml_node<> *object = del->first_node(object_s, strlen(object_s), false);
       object; object = object->next_sibling(object_s, strlen(object_s), false)) {
    xml_node<> *key = object->first_node(key_s, strlen(key_s), false);
    if (!key) {
      return false;
    }
    keys->push_back(key->value());
  }

  return true;
}

inline std::string ExtraParNum(std::string object_name, std::string subobject_name) {
  return subobject_name.substr(3, subobject_name.size() - 32 - object_name.size());
}

std::string iso8601_time(time_t sec, suseconds_t usec) {
  int milli = usec / 1000;
  char buf[128] = {0};
  strftime(buf, sizeof buf, "%FT%T", gmtime(&sec));
  sprintf(buf, "%s.%03dZ", buf, milli);
  return std::string(buf);
}

}  // namespace xml
