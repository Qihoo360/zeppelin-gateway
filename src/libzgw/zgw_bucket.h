#ifndef ZGW_BUCKET_H
#define ZGW_BUCKET_H
#include <string>
#include <map>

#include "zgw_object.h"

namespace libzgw {

class ZgwBucket {
public:
  ZgwBucket(const std::string name) {}
  ~ZgwBucket() {}
 
  ZgwObject* GetObject(uint64_t id);
  

private:
  std::string name_;
  //std::map<uint64_t, ZgwObject*> objects_;
};


}

#endif
