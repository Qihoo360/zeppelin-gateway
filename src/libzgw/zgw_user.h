#ifndef ZGW_USER_H
#define ZGW_USER_H
#include <string>

#include "time.h"

namespace libzgw {

struct ZgwUserInfo {
  int id;
  std::string disply_name;
};

class ZgwUser {
public:
  ZgwUser(int uid, const std::string& name) {
    info_.id = uid;
    info_.disply_name = name;
  }
  ~ZgwUser() {}

private:
  ZgwUserInfo info_;
};

}

#endif
