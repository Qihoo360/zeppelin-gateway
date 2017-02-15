#ifndef ZGW_OBJECT_H
#define ZGW_OBJECT_H
#include <set>
#include <string>

namespace libzgw {

class ZgwObjectMeta {

};

class ZgwObject {
public:
  ZgwObject(uint64_t id);
  ~ZgwObject();

private:
  uint64_t id_;
  ZgwObjectMeta meta_;
  std::set<std::string> keys_;
};

}

#endif
