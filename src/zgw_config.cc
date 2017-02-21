#include "zgw_config.h"
#include <iostream>

ZgwConfig::ZgwConfig(std::string path)
      : zp_meta_ip_port(""),
        server_ip("0.0.0.0"),
        server_port(8099),
        daemonize(false) {
  b_conf = new slash::BaseConf(path);
}

ZgwConfig::~ZgwConfig() {
}

int ZgwConfig::LoadConf() {
  if (b_conf->LoadConf() != 0)
    return -1;

  b_conf->GetConfStr("zp_meta_addr", &zp_meta_ip_port);
  b_conf->GetConfStr("server_ip", &server_ip);
  b_conf->GetConfInt("server_port", &server_port);
  b_conf->GetConfBool("daemonize", &daemonize);
  b_conf->GetConfStr("log_path", &log_path);

  return 0;
}

void ZgwConfig::Dump() {
  std::cout << "zp_meta_addr: " << zp_meta_ip_port<< std::endl;
  std::cout << "server_ip: " << server_ip << std::endl;
  std::cout << "server_port: " << server_port << std::endl;
  std::cout << "daemonize: " << daemonize << std::endl;
  std::cout << "log_path: " << log_path << std::endl;
}
