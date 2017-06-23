#ifndef ZGW_STORE_GC_H_
#define ZGW_STORE_GC_H_

#include <iostream>
#include <string>

#include "zgw_store.h"
#include "pink/include/pink_thread.h"
#include "slash/include/env.h"


namespace zgwstore {

class GCThread : public pink::Thread {
 public:
  GCThread()
      : latest_action_time_(slash::NowMicros()) {
    set_thread_name("GCThread");
  }
  ~GCThread() {}

  int StartThread(ZgwStore* store) {
    store_ = store;
    return Thread::StartThread();
  }

  std::string GCStatus();

 private:
  virtual void* ThreadMain() override;
  Status ParseDeletedBlocks(const std::string& deleted_item,
                          std::vector<std::string>* block_indexs);

  uint64_t latest_action_time_;
  ZgwStore* store_;
};

}  // namespace zgwstore

#endif
