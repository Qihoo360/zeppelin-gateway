#ifndef ZGW_CONST_H
#define ZGW_CONST_H

#include <string>

const int kMaxWorkerThread = 100;

#define dstr(a) #a
#define dxstr(a) dstr(a)
const std::string kZgwVersion = dxstr(_GITVER_);
const std::string kZgwCompileDate = dxstr(_COMPILEDATE_);
const std::string kZgwPidFile = "zgw.pid";
const std::string kZgwLockFile = "zgw.lock";

////// Server State /////
// const int kZgwCronCount = 30;
const int kZgwCronInterval = 2000000; // 2s
// const int kZgwRedisLockTTL = 300000; // 5 minutes
const int kZgwRedisLockTTL = 30000; // 30s

#endif
