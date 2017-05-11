#ifndef ZGW_S3_AUTHV4_H
#define ZGW_S3_AUTHV4_H

#include <iostream>
#include <string>

#include "pink/include/http_conn.h"
#include "src/zgwstore/zgw_store.h"

enum AuthErrorType {
  kAuthSuccess,
  kAccessKeyInvalid,
  kSignatureNotMatch,
  kMaybeAuthV2,
};

class S3AuthV4 {
 public:
  void Initialize(const pink::HttpRequest* req, zgwstore::ZgwStore* store);

  AuthErrorType TryAuth();
  std::string user_name();

  std::string ToString();

 private:
  friend class S3Cmd;

  struct Rep;
  Rep* rep_;

  S3AuthV4();
  ~S3AuthV4();

  // No copying allowed
  S3AuthV4(const S3AuthV4&);
  S3AuthV4& operator=(const S3AuthV4&);
};

#endif
