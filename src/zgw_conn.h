#ifndef ZGW_CONN_H
#define ZGW_CONN_H

#include "include/pink_thread.h"
#include "include/http_conn.h"
#include "zgw_store.h"

class ZgwConn : public pink::HttpConn {
 public:
  ZgwConn(const int fd, const std::string &ip_port,
          pink::WorkerThread<ZgwConn>* worker);

 private:
  virtual void DealMessage(const pink::HttpRequest* req,
                           pink::HttpResponse* res) override;

  libzgw::ZgwStore *store_;
  void GetObjectHandle(std::string &bucket_name,
                       std::string &object_name,
                       pink::HttpResponse* resp);
  void PutObjectHandle(const pink::HttpRequest *req,
                       std::string &bucket_name,
                       std::string &object_name,
                       pink::HttpResponse* resp);
  void DelObjectHandle(std::string &bucket_name,
                       std::string &object_name,
                       pink::HttpResponse* resp);
  void ListObjectHandle(std::string &bucket_name,
                        pink::HttpResponse* resp);

  void PutBucketHandle(std::string &bucket_name,
                       pink::HttpResponse* resp);
  void DelBucketHandle(std::string &bucket_name,
                       pink::HttpResponse* resp);
  void ListBucketHandle(pink::HttpResponse* resp);

  std::string iso8601_time(time_t sec, suseconds_t usec = 0);
};


#endif
