#ifndef ZGW_CONST_H
#define ZGW_CONST_H

#include <string>

const int kMaxWorkerThread = 100;

#define str(a) #a
#define xstr(a) str(a)
const std::string kZgwVersion = xstr(_GITVER_);
const std::string kZgwCompileDate = xstr(_COMPILEDATE_);
const std::string kZgwPidFile = "zgw.pid";
const std::string kZgwLockFile = "zgw.lock";

////// Server State /////
// const int kZgwCronCount = 30;
const int kZgwCronInterval = 1000000; // us
const int kDispatchCronInterval = 1; // ms

#endif
