#include "src/zgw_config.h"

#include <iostream>

#include "slash/include/slash_string.h"
#include "src/zgw_const.h"

static const std::string kZgwTableName = "__zgw20_data_table";

ZgwConfig::ZgwConfig(std::string path)
      : zp_table_name(kZgwTableName),
        zp_optimeout_ms(5000),  // 5 seconds
        redis_ip_port("127.0.0.1"),
        redis_passwd("_"),
        server_ip("0.0.0.0"),
        server_port(8099),
        keepalive_timeout(30),
        admin_port(8199),
        daemonize(false),
        minloglevel(0),
        worker_num(2),
        max_clients(5000),
        enable_gc(false),
        log_path("./log"),
        pid_file(kZgwPidFile),
        enable_security(false),
        cert_file("zgw.crt"),
        key_file("zgw.key") {
  b_conf = new slash::BaseConf(path);
}

ZgwConfig::~ZgwConfig() {
  delete b_conf;
}

int ZgwConfig::LoadConf() {
  if (b_conf->LoadConf() != 0)
    return -1;

  // Meta server
  std::string zp_meta_addr;
  b_conf->GetConfStr("zp_meta_addr", &zp_meta_addr);
  slash::StringSplit(zp_meta_addr, '/', zp_meta_ip_ports);
  b_conf->GetConfInt("zp_optimeout_ms", &zp_optimeout_ms);
  b_conf->GetConfStr("redis_ip_port", &redis_ip_port);
  b_conf->GetConfStr("redis_passwd", &redis_passwd);
  // b_conf->GetConfStr("zp_table_name", &zp_table_name);

  // Server info
  b_conf->GetConfStr("server_ip", &server_ip);
  b_conf->GetConfInt("server_port", &server_port);
  b_conf->GetConfInt("keepalive_timeout", &keepalive_timeout);
  b_conf->GetConfInt("admin_port", &admin_port);
  b_conf->GetConfBool("daemonize", &daemonize);
  b_conf->GetConfInt("minloglevel", &minloglevel);
  b_conf->GetConfInt("worker_num", &worker_num);
  b_conf->GetConfInt("max_clients", &max_clients);
  b_conf->GetConfBool("enable_gc", &enable_gc);

  b_conf->GetConfStr("log_path", &log_path);
  b_conf->GetConfStr("pid_file", &pid_file);

  // Security relative
  b_conf->GetConfBool("enable_security", &enable_security);
  b_conf->GetConfStr("cert_file", &cert_file);
  b_conf->GetConfStr("key_file", &key_file);

  return 0;
}

void ZgwConfig::Dump() {
  b_conf->DumpConf();
}
