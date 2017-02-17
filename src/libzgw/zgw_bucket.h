#ifndef ZGW_BUCKET_H
#define ZGW_BUCKET_H
#include <string>
#include <map>
#include <time.h>

#include "include/slash_string.h"
#include "include/slash_status.h"

namespace libzgw {

using slash::Status;

class ZgwBucket {
public:
  ZgwBucket(const std::string& name);
  ~ZgwBucket();
  
  std::string name() const {
    return name_;
  }

  std::string MetaKey() const;
  std::string MetaValue() const;
  Status ParseMetaValue(const std::string& value);
 
private:
  std::string name_;
  time_t ctime_;
};


}

#endif
