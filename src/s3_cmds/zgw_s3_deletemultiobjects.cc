#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_xml.h"
#include "src/zgwstore/zgw_define.h"

bool DeleteMultiObjectsCmd::DoInitial() {
  http_request_xml_.clear();
  http_response_xml_.clear();
  request_id_ = md5(bucket_name_ +
                    std::to_string(slash::NowMicros()));
  if (!TryAuth()) {
    DLOG(ERROR) << request_id_ << " " <<
      "DeleteMultiObjects(DoInitial) - Auth failed: " << client_ip_port_;
    return false;
  }

  DLOG(INFO) << request_id_ << " " <<
    "DeleteMultiObjects(DoInitial) - " << bucket_name_;
  return true;
}

void DeleteMultiObjectsCmd::DoReceiveBody(const char* data, size_t data_size) {
  http_request_xml_.append(data, data_size);
}

static bool ParseReqXml(const std::string& xml_str,
                        std::vector<std::string>* objects_to_delete) {
  S3XmlDoc doc;
  if (!doc.ParseFromString(xml_str)) {
    return false;
  }
  S3XmlNode del, obj;
  if (!doc.FindFirstNode("Delete", &del)) {
    return false;
  }
  if (!del.FindFirstNode("Object", &obj)) {
    return false;
  }
  do {
    S3XmlNode key;
    if (!obj.FindFirstNode("Key", &key)) {
      return false;
    }
    objects_to_delete->push_back(key.value());
  } while (obj.NextSibling());

  return true;
}

void DeleteMultiObjectsCmd::DoAndResponse(pink::HTTPResponse* resp) {
  if (http_ret_code_ == 200) {
    std::vector<std::string> objects_to_delete;
    if (!ParseReqXml(http_request_xml_, &objects_to_delete)) {
      http_ret_code_ = 400;
      GenerateErrorXml(kMalformedXML);
    }

    if (http_ret_code_ == 200) {
      zgwstore::Bucket dummy_bk;
      Status s = store_->GetBucket(user_name_, bucket_name_, &dummy_bk);
      if (!s.ok()) {
        http_ret_code_ = 404;
        GenerateErrorXml(kNoSuchBucket, bucket_name_);
      }
    }

    if (http_ret_code_ == 200) {
      S3XmlDoc doc("DeleteResult");
      for (auto& obj : objects_to_delete) {
        Status s = store_->DeleteObject(user_name_, bucket_name_, obj);
        if (s.ok()) {
          S3XmlNode* deleted_node = doc.AllocateNode("Deleted");
          deleted_node->AppendNode("Key", obj);
          doc.AppendToRoot(deleted_node);
        } else {
          S3XmlNode* error_node = doc.AllocateNode("Error");
          error_node->AppendNode("Key", obj);
          error_node->AppendNode("Code", "");
          error_node->AppendNode("Message", "");
          doc.AppendToRoot(error_node);
        }
      }
      doc.ToString(&http_response_xml_);
    }
  }

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int DeleteMultiObjectsCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
