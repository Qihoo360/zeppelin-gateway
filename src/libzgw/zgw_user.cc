#include "zgw_user.h"

namespace libzgw {

std::string ZgwUser::MetaValue() const {
  std::string result;
  // User Internal meta
  slash::PutFixed32(&result, id_);
  slash::PutLengthPrefixedString(&result, disply_name_);
  return result;
}

Status ZgwUser::ParseMetaValue(std::string* value) {
  // User Interal meta
  slash::GetFixed32(value, &id_);
  bool res = slash::GetLengthPrefixedString(value, &(disply_name_)); // etag
  if (!res) {
    return Status::Corruption("Parse disply_name failed");
  }
  return Status::OK();
}

}
