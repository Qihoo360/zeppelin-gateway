#ifndef ZGW_BUCKET_H
#define ZGW_BUCKET_H
#include <string>
#include <map>
#include <sys/time.h>

#include "include/slash_string.h"
#include "include/slash_status.h"

namespace libzgw {

using slash::Status;

class ZgwBucket {
public:
  explicit ZgwBucket(const std::string& name);
  ~ZgwBucket();
  
  std::string name() const {
    return name_;
  }

  timeval ctime() {
    return ctime_;
  }

  uint32_t object_count() {
    return object_count_;
  }

  static std::string MetaKey(std::string name);
  std::string MetaValue() const;
  // this may change value inside
  Status ParseMetaValue(std::string& value);
 
private:
  std::string name_;
  timeval ctime_;
  uint32_t object_count_;
};


}

#endif
