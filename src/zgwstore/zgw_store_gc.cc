#include "zgw_store_gc.h"

#include <unistd.h>
#include <algorithm>

#include <glog/logging.h>

namespace zgwstore {

static const int kGCIntervalSeconds = 1; // 1 second
static const int kBlockReservedTime = 1 * 60 * 60 ; // 1 hour
// static const int kBlockReservedTime = 1 * 60; // 1 minute

std::string GCThread::GCStatus() {
  // TODO gdq
  return "";
}

Status GCThread::ParseDeletedBlocks(const std::string& deleted_item,
                                  std::vector<std::string>* block_indexs) {
  Status s;
  // deleted_item: 84788d7a9282d8c0109a44b6d9c06887testbk1|ob1
  if (deleted_item.size() > 32 &&
      std::count(deleted_item.begin(), deleted_item.end(), '|') == 1) {
    std::string upload_id = deleted_item.substr(0, 32);
    size_t sep_pos = deleted_item.find('|');
    std::string bkname = deleted_item.substr(32, sep_pos - 32);
    std::string obname = deleted_item.substr(sep_pos + 1);
    std::vector<std::string> tmp;
    s = store_->GetMultiBlockSet(bkname, obname, upload_id, &tmp);
    if (!s.ok()) {
      return s;
    }
    block_indexs->clear();
    for (auto& b : tmp) {
      // Trim sort sign e.g. 00012(3-9)(0,12) -> (3-9)(0,12)
      b = b.substr(5);
      std::vector<std::string> items;
      slash::StringSplit(b, '|', items);
      for (auto& i : items) {
        block_indexs->push_back(i);
      }
    }
    s = store_->DeleteMultiBlockSet(bkname, obname, upload_id);
    if (!s.ok()) {
      LOG(ERROR) << "DeleteMultiBlockSet " << bkname << "/" << obname << "_" <<
        upload_id << " error: " << s.ToString();
      return s;
    }
  }
  // deleted_item: 
  //    1235-1235(0,258)
  //    1235-1235(0,258)|1236-1236(0,258)
  int lbracket = std::count(deleted_item.begin(), deleted_item.end(), '(');
  int rbracket = std::count(deleted_item.begin(), deleted_item.end(), ')');
  if (block_indexs->empty() &&
      lbracket != 0 &&
      lbracket == rbracket) {
    std::vector<std::string> items;
    slash::StringSplit(deleted_item, '|', items);
    for (auto& i : items) {
      block_indexs->push_back(i);
    }
  }

  return Status::OK();
}

void* GCThread::ThreadMain() {
  while (!should_stop()) {
    uint64_t now = slash::NowMicros();
    if (now - latest_action_time_ < kGCIntervalSeconds * 1e6) {
      sleep(kGCIntervalSeconds);
      continue;
    }
    latest_action_time_ = now;

    std::string item, deleted_blocks;
    uint64_t deleted_time;
    Status s = store_->GetDeletedItem(&item);
    if (!s.ok()) {
      if (s.IsNotFound()) {
        continue;
      }
      LOG(ERROR) << "GetDeteledItem error: " << item << " " <<
        s.ToString();
      // TODO (gaodq)
      return nullptr;
    }
    size_t slash_pos = item.find('/');
    if (slash_pos == std::string::npos) {
      LOG(WARNING) << "Unknow format: " << item;
      continue;
    }
    // e.g.       deleted_blocks     deleted_time
    //      item: 1235-1235(0,258)/1498186110766016
    //            1235-1235(0,258)|1236-1236(0,258)/1498186110766016
    //            84788d7a9282d8c0109a44b6d9c06887testbk1|ob1/1498186110766016
    deleted_time = std::atol(item.substr(slash_pos + 1).c_str());
    deleted_blocks = item.substr(0, slash_pos);
    if (now - deleted_time < kBlockReservedTime * 1e6) {
      s = store_->PutDeletedItem(deleted_blocks, deleted_time);
      if (!s.ok()) {
        LOG(ERROR) << "PutDeletedItem error: " << item << " " <<
          s.ToString();
        return nullptr;
      }
      continue;
    }

    std::vector<std::string> block_indexs;
    int deleted_count_ = 0;
    s = ParseDeletedBlocks(deleted_blocks, &block_indexs);
    if (!s.ok()) {
      LOG(ERROR) << "ParseDeletedBlocks error: " << deleted_blocks << " " <<
        s.ToString();
      return nullptr;
    }

    for (auto& index : block_indexs) {
      LOG(INFO) << "Delete block: " << index;

      uint64_t start_block, end_block, dummy1, dummy2;
      sscanf(index.c_str(), "%lu-%lu(%lu,%lu)",
             &start_block, &end_block, &dummy1, &dummy2);

      s = store_->Lock();
      if (!s.ok()) {
        LOG(ERROR) << "GCThread Lock error: " << s.ToString();
      }
      for (uint64_t b = start_block; b <= end_block; b++) {
        s = store_->BlockUnref(b);
        if (!s.ok()) {
          LOG(ERROR) << "BlockUnref Block " << b << " error: " << s.ToString();
        }
      }
      s = store_->UnLock();
      if (!s.ok()) {
        LOG(ERROR) << "GCThread UnLock error: " << s.ToString();
      }
    }
  }
  return nullptr;
}

}  // namespace zgwstore
