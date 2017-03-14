#ifndef ZGW_CONN_H
#define ZGW_CONN_H

#include "include/pink_thread.h"
#include "include/http_conn.h"
#include "zgw_store.h"

class ZgwWorkerThread;

class ZgwConn : public pink::HttpConn {
 public:
  ZgwConn(const int fd, const std::string &ip_port,
          pink::Thread* worker);

 private:
  virtual void DealMessage(const pink::HttpRequest* req,
                           pink::HttpResponse* res) override;

  // Operation On Objects
  void GetObjectHandle(bool is_head_op = false);
  void PutObjectHandle();
  void DelObjectHandle();

  void InitialMultiUpload();
  void UploadPartHandle();
  void ListParts();
  void CompleteMultiUpload();
  void AbortMultiUpload();

  // Operation On Buckets
  void PutBucketHandle();
  void DelBucketHandle();
  void ListObjectHandle(bool is_head_op = false);
  void ListMultiPartsUpload();

  // Operation On Service
  void ListBucketHandle();
  void ListUsersHandle();

 private:
  enum METHOD {
    GET,
    PUT,
    DELETE,
    HEAD,
    POST,
    UNSUPPORT,
  };

  ZgwWorkerThread *worker_;
  libzgw::ZgwStore *store_;

  // Parse from http request
  std::string access_key_;
  pink::HttpRequest *req_;
  pink::HttpResponse *resp_;
  std::string bucket_name_;
  std::string object_name_;

  // Get from zp
  libzgw::NameList *buckets_name_;
  libzgw::NameList *objects_name_;
  libzgw::ZgwUser *zgw_user_;

  std::string GetAccessKey();

  bool IsBucketOp() {
    return (!bucket_name_.empty() && object_name_.empty());
  }

  bool IsObjectOp() {
    return (!bucket_name_.empty() && !object_name_.empty());
  }
};


#endif
