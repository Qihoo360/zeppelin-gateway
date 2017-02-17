#ifndef ZGW_CONN_H
#define ZGW_CONN_H

#include "include/pink_thread.h"
#include "include/http_conn.h"

class ZgwConn : public pink::HttpConn {
 public:
  ZgwConn(const int fd, const std::string &ip_port,
      pink::WorkerThread<ZgwConn>* worker);

 private:
  virtual void DealMessage(const pink::HttpRequest* req,
      pink::HttpResponse* res) override;

  void ListObjectOperation(std::string &bucket_name,
                           pink::HttpResponse* resp);
  void DeleteBucketOperation(std::string &bucket_name,
                             pink::HttpResponse* resp);
  void CreateBucketOperation(std::string &bucket_name,
                             pink::HttpResponse* resp);
  void ListBucketOperation(pink::HttpResponse* resp);
};


#endif
