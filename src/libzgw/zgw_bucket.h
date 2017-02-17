#ifndef ZGW_BUCKET_H
#define ZGW_BUCKET_H
#include <string>
#include <map>

#include "zgw_object.h"

namespace libzgw {

class ZgwBucket {
public:
  ZgwBucket(const std::string name, const std::string date)
      : name_(name),
        date_(date) {
  }
  ~ZgwBucket() {}
 
  ZgwObject* GetObject(uint64_t id);
  
  std::string GetName() {
    return name_;
  }

  std::string GetDate() {
    return date_;
  }

private:
  std::string name_;
  std::string date_;
  //std::map<uint64_t, ZgwObject*> objects_;
};


}

#endif
