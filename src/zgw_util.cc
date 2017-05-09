#include "src/zgw_util.h"

#include <sys/time.h>

#include <openssl/md5.h>

void SplitBySecondSlash(const std::string& req_path,
                        std::string* field1,
                        std::string* field2) {
  std::string _path;
  if (req_path.empty()) {
    return;
  } else if (req_path.at(0) != '/') {
    _path = "/" + req_path;
  } else {
    _path = req_path;
  }

  size_t pos = _path.find('/', 1);
  if (pos == std::string::npos) {
    field1->assign(_path.substr(1));
    field2->clear();
  } else {
    field1->assign(_path.substr(1, pos - 1));
    field2->assign(_path.substr(pos + 1));
  }
  if (!field2->empty() && field2->back() == '/') {
    field2->erase(field2->size() - 1);
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
