#ifndef ZGW_USER_H
#define ZGW_USER_H
#include <string>
#include "time.h"
#include "include/slash_status.h"
#include "include/slash_coding.h"


namespace libzgw {

using slash::Status;

class ZgwUser {
public:
  ZgwUser(uint32_t uid, const std::string& name)
    : id_(uid),
    disply_name_(name){
    }
  ZgwUser()
    : id_(0) {
    }
  ~ZgwUser() {}

  // Serialization
  std::string MetaValue() const;
  
  // Deserialization
  Status ParseMetaValue(std::string* value);

private:
  uint32_t id_;
  std::string disply_name_;
};

}

#endif
