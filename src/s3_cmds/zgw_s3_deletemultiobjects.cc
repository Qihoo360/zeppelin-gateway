#include "src/s3_cmds/zgw_s3_bucket.h"

#include "slash/include/env.h"
#include "src/zgwstore/zgw_define.h"

bool DeleteMultiObjectsCmd::DoInitial() {

  return true;
}

void DeleteMultiObjectsCmd::DoReceiveBody(const char* data, size_t data_size) {
}

void DeleteMultiObjectsCmd::DoAndResponse(pink::HttpResponse* resp) {

  resp->SetStatusCode(http_ret_code_);
  resp->SetContentLength(http_response_xml_.size());
}

int DeleteMultiObjectsCmd::DoResponseBody(char* buf, size_t max_size) {
  if (max_size < http_response_xml_.size()) {
    memcpy(buf, http_response_xml_.data(), max_size);
    http_response_xml_.assign(http_response_xml_.substr(max_size));
  } else {
    memcpy(buf, http_response_xml_.data(), http_response_xml_.size());
  }

  return std::min(max_size, http_response_xml_.size());
}
