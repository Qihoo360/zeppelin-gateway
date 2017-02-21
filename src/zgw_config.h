#ifndef ZGW_CONFIG_H
#define ZGW_CONFIG_H

#include <string>

#include "base_conf.h"

struct ZgwConfig {
  explicit ZgwConfig(std::string path);
  ~ZgwConfig();
  int LoadConf();
  void Dump();

  slash::BaseConf *b_conf;

  std::string zp_meta_ip_port;
  std::string server_ip;
  int server_port;
  bool daemonize;

  std::string log_path;
};

#endif
