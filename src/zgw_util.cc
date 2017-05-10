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

static inline void char2hex(unsigned char c, unsigned char &hex1, unsigned char &hex2) {
    hex1 = c / 16;
    hex2 = c % 16;
    hex1 += hex1 <= 9 ? '0' : 'a' - 10;
    hex2 += hex2 <= 9 ? '0' : 'a' - 10;
}

static inline char hex2char(const std::string& hex) {
  assert(hex.size() == 2);
  char a = std::tolower(hex[0]);
  char b = std::tolower(hex[1]);
  a = a >= 'a' ? (10 + a - 'a') : (a - '0');
  b = b >= 'a' ? (10 + b - 'a') : (b - '0');
  return (a * 16 + b);
}

std::string UrlDecode(const std::string& url) {
  std::string v;
  size_t i = 0;
  while (i < url.size()) {
    if (url[i] == '%') {
      if (i + 3 > url.size()) {
        break;
      }
      char c = hex2char(url.substr(i + 1, 2));
      v.push_back(c);
      i += 3;
    } else {
      v.push_back(url[i++]);
    }
  }
  return v;
}

std::string UrlEncode(const std::string& s, bool encode_slash) {
  const char *str = s.c_str();
  std::string v;
  v.clear();
  for (size_t i = 0, l = s.size(); i < l; i++) {
    char c = str[i];
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c == '_' || c == '-' || c == '~' || c == '.')) {
      v.push_back(c);
    } else if (c == '/') {
      if (encode_slash) {
        v.append("%2F");
      } else {
        v.push_back(c);
      }
    } else if (c == ' ') {
      v.append("%20");
    } else {
      v.push_back('%');
      unsigned char d1, d2;
      char2hex(c, d1, d2);
      v.push_back(d1);
      v.push_back(d2);
    }
  }

  return v;
}
