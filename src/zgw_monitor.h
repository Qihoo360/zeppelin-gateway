#ifndef ZGW_MONITOR_H
#define ZGW_MONITOR_H

#include <atomic>
#include <string>

#include "slash/include/env.h"
#include "src/s3_cmds/zgw_s3_command.h"

class ZgwMonitor {
 public:
  ZgwMonitor()
      : last_time_us_(slash::NowMicros()) {
  }

  void AddBucketTraffic(const std::string& bk_name, uint64_t size);
  void AddApiRequest(S3Commands cmd_type, int error_code);
  void UpdateUpPartTime(double time);
  void AddQueryNum();
  uint64_t UpdateAndGetQPS();
  void AddAuthFailed();

  uint64_t cluster_traffic();
  uint64_t bucket_traffic(const std::string& bk_name);
  uint64_t api_request_count(S3Commands cmd_type);
  uint64_t api_err4xx_count(S3Commands cmd_type);
  uint64_t api_err5xx_count(S3Commands cmd_type);
  uint64_t request_count();
  uint64_t failed_request();
  uint64_t avg_upload_part_time();
  uint64_t auth_failed_count();

 private:
  std::atomic<uint64_t> cluster_traffic_;
  std::map<std::string, uint64_t> bucket_traffic_;
  std::map<S3Commands, uint64_t> api_request_count_;
  std::map<S3Commands, uint64_t> api_err4xx_count_;
  std::map<S3Commands, uint64_t> api_err5xx_count_;
  std::atomic<uint64_t> request_count_;
  std::atomic<uint64_t> failed_request_;
  std::atomic<uint64_t> avg_upload_part_time_;
  std::atomic<uint64_t> auth_failed_count_;

  // QPS
  uint64_t last_query_num_;
  std::atomic<uint64_t> cur_query_num_;
  uint64_t last_time_us_;
};

#endif
