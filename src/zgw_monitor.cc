#include "src/zgw_monitor.h"

void ZgwMonitor::AddBucketTraffic(const std::string& bk_name, uint64_t size) {
  if (bucket_traffic_.count(bk_name)) {
    bucket_traffic_[bk_name] += size;
  } else {
    bucket_traffic_[bk_name] = size;
  }

  if (cluster_traffic_.load() == 0) {
    cluster_traffic_ = size;
  } else {
    cluster_traffic_ += size;
  }
}

void ZgwMonitor::AddAuthFailed() {
  auth_failed_count_++;
  failed_request_++;
}

void ZgwMonitor::AddApiRequest(S3Commands cmd_type, int error_code) {
  request_count_++;
  if (api_request_count_.count(cmd_type)) {
    api_request_count_[cmd_type] += 1;
  } else {
    api_request_count_[cmd_type] = 1;
  }

  if (error_code >= 400 && error_code < 500) {
    if (api_err4xx_count_.count(cmd_type)) {
      api_err4xx_count_[cmd_type] += 1;
    } else {
      api_err4xx_count_[cmd_type] = 1;
    }
    failed_request_++;
  } else if (error_code >= 500) {
    if (api_err5xx_count_.count(cmd_type)) {
      api_err5xx_count_[cmd_type] += 1;
    } else {
      api_err5xx_count_[cmd_type] = 1;
    }
    failed_request_++;
  }
}

void ZgwMonitor::UpdateUpPartTime(double time) {
  // Maybe less precise
  static uint64_t n = 1; // Sequence of data flow
  // Avg(n)=(n-1)/n*Avg(n-1)+data[n]/n
  avg_upload_part_time_ =
    static_cast<double>((n - 1) * avg_upload_part_time_) / n + time / n;
  ++n;
}

void ZgwMonitor::AddQueryNum() {
  cur_query_num_++;
}

uint64_t ZgwMonitor::UpdateAndGetQPS() {
  uint64_t cur_time_us = slash::NowMicros();
  uint64_t qps = (cur_query_num_ - last_query_num_) * 1000000
    / (cur_time_us - last_time_us_ + 1);
  if (qps == 0) {
    cur_query_num_ = 0;
  }
  last_query_num_ = cur_query_num_;
  last_time_us_ = cur_time_us;
  return qps;
}

uint64_t ZgwMonitor::cluster_traffic() {
  return cluster_traffic_.load();
}

uint64_t ZgwMonitor::bucket_traffic(const std::string& bk_name) {
  if (bucket_traffic_.count(bk_name)) {
    return bucket_traffic_.at(bk_name);
  } else {
    return 0;
  }
}

uint64_t ZgwMonitor::api_request_count(S3Commands cmd_type) {
  if (api_request_count_.count(cmd_type)) {
    return api_request_count_.at(cmd_type);
  } else {
    return 0;
  }
}

uint64_t ZgwMonitor::api_err4xx_count(S3Commands cmd_type) {
  if (api_err4xx_count_.count(cmd_type)) {
    return api_err4xx_count_.at(cmd_type);
  } else {
    return 0;
  }
}

uint64_t ZgwMonitor::api_err5xx_count(S3Commands cmd_type) {
  if (api_err5xx_count_.count(cmd_type)) {
    return api_err5xx_count_.at(cmd_type);
  } else {
    return 0;
  }
}

uint64_t ZgwMonitor::request_count() {
  return request_count_;
}

uint64_t ZgwMonitor::failed_request() {
  return failed_request_;
}

uint64_t ZgwMonitor::avg_upload_part_time() {
  return avg_upload_part_time_;
}

uint64_t ZgwMonitor::auth_failed_count() {
  return auth_failed_count_;
}

