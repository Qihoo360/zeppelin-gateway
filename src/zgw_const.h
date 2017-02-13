#ifndef ZGW_CONST_H
#define ZGW_CONST_H

#include <string>

const int kMaxWorkerThread = 100;

const std::string kZgwVersion = "0.0.1";
const std::string kZgwPidFile = "zgw.pid";
const std::string kZgwLockFile = "zgw.lock";

////// Server State /////
const int kDispatchCronInterval = 5000;
const int kWorkerCronInterval = 5000;

#endif
