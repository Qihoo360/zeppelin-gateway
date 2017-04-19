#include "src/zgw_util.h"

#include <sys/time.h>

#include <openssl/md5.h>

void ExtraBucketAndObject(const std::string& _path,
                          std::string* bucket_name, std::string* object_name) {
  std::string path = _path;
  if (path[0] != '/') {
    path = "/" + path;
  }
  size_t pos = path.find('/', 1);
  if (pos == std::string::npos) {
    bucket_name->assign(path.substr(1));
    object_name->clear();
  } else {
    bucket_name->assign(path.substr(1, pos - 1));
    object_name->assign(path.substr(pos + 1));
  }
  if (!object_name->empty() && object_name->back() == '/') {
    object_name->erase(object_name->size() - 1);
  }
}

std::string http_nowtime(time_t t) {
  char buf[100] = {0};
  struct tm t_ = *gmtime(&t);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &t_);
  return std::string(buf);
}

std::string md5(const std::string& content) {
  MD5_CTX md5_ctx;
  char buf[33] = {0};
  unsigned char md5[16] = {0};
  MD5_Init(&md5_ctx);
  MD5_Update(&md5_ctx, content.c_str(), content.size());
  MD5_Final(md5, &md5_ctx);
  for (int i = 0; i < 16; i++) {
    sprintf(buf + i * 2, "%02x", md5[i]);
  }
  return std::string(buf);
}

void DumpHttpRequest(const pink::HttpRequest* req) {
  DLOG(INFO) << "handle get"<< std::endl;
  DLOG(INFO) << " + method: " << req->method;
  DLOG(INFO) << " + path: " << req->path;
  DLOG(INFO) << " + version: " << req->version;
  if (req->content.size() > 50) {
    DLOG(INFO) << " + content: " << req->content.substr(0, 50);
  } else {
    DLOG(INFO) << " + content: " << req->content;
  }
  DLOG(INFO) << " + headers: ";
  DLOG(INFO) << "------------------------------------- ";
  for (auto h : req->headers) {
    DLOG(INFO) << "   + " << h.first << "=>" << h.second;
  }
  DLOG(INFO) << "------------------------------------- ";
  DLOG(INFO) << "------------------------------------- ";
  DLOG(INFO) << " + query_params: ";
  for (auto q : req->query_params) {
    DLOG(INFO) << "   + " << q.first << "=>" << q.second;
  }
  DLOG(INFO) << "------------------------------------- ";
} 
