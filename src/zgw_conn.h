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
  ZgwWorkerThread *worker_;

  libzgw::ZgwStore *store_;
  void GetObjectHandle(const pink::HttpRequest* req,
                       std::string &bucket_name,
                       std::string &object_name,
                       pink::HttpResponse* resp);
  void PutObjectHandle(const pink::HttpRequest *req,
                       std::string &bucket_name,
                       std::string &object_name,
                       pink::HttpResponse* resp);
  void DelObjectHandle(const pink::HttpRequest* req,
                       std::string &bucket_name,
                       std::string &object_name,
                       pink::HttpResponse* resp);
  void ListObjectHandle(const pink::HttpRequest* req,
                        std::string &bucket_name,
                        pink::HttpResponse* resp);

  void PutBucketHandle(const pink::HttpRequest* req,
                       std::string &bucket_name,
                       pink::HttpResponse* resp);
  void DelBucketHandle(const pink::HttpRequest* req,
                       std::string &bucket_name,
                       pink::HttpResponse* resp);
  void ListBucketHandle(const pink::HttpRequest *req, pink::HttpResponse* resp);

  void AuthFailedHandle(pink::HttpResponse* resp);
  std::string iso8601_time(time_t sec, suseconds_t usec = 0);
  std::string GetAccessKey(const pink::HttpRequest* req);
};


#endif
