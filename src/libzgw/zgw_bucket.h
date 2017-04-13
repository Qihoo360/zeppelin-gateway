#ifndef ZGW_BUCKET_H
#define ZGW_BUCKET_H

#include <string>
#include <set>
#include <sys/time.h>

#include "slash/include/slash_string.h"
#include "slash/include/slash_status.h"
#include "zgw_user.h"

namespace libzgw {

using slash::Status;

class ZgwBucket {
 public:
  explicit ZgwBucket(const std::string& name);
  ~ZgwBucket();
  
  std::string name() const {
    return name_;
  }

  const timeval ctime() const {
    return ctime_;
  }

  ZgwUserInfo user_info() {
    return user_info_;
  }

  void SetUserInfo(const ZgwUserInfo &user_info) {
    user_info_ = user_info;
  }

  std::string MetaKey() const;
  std::string MetaValue() const;
  // this may change value inside
  Status ParseMetaValue(std::string& value);
 
 private:
  ZgwUserInfo user_info_;
  std::string name_;
  timeval ctime_;
};

}  // namespace libzgw

#endif
