// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#include <string>
#include <iostream>

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <glog/logging.h>

#include "include/env.h"
#include "include/slash_string.h"
#include "zgw_server.h"
#include "zgw_conf.h"

ZgwServer* g_zgw_server;

static void GlogInit(ZgwConf *zgw_conf) {
  std::string log_path = zgw_conf->log_path;
  if (!slash::FileExists(log_path)) {
    slash::CreatePath(log_path);
  }

  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = log_path;
  FLAGS_minloglevel = 0;
  FLAGS_max_log_size = 1800;

  ::google::InitGoogleLogging("zgw");
}

static void IntSigHandle(const int sig) {
  LOG(INFO) << "Catch Signal " << sig << ", cleanup...";
  g_zgw_server->Exit();
  LOG(INFO) << "zgw server Exit";
}

static void ZgwSignalSetup() {
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, &IntSigHandle);
  signal(SIGQUIT, &IntSigHandle);
  signal(SIGTERM, &IntSigHandle);
}

void Usage() {
  printf("Usage:\n"
          "  ./zgw -c [config file]\n");
}

void ZgwConfInit(ZgwConf **zgw_conf, int argc, char* argv[]) {
  if (argc < 1) {
    Usage();
    exit(-1);
  }
  bool path_opt = false;
  char c;
  char path[1024];
  while (-1 != (c = getopt(argc, argv, "c:h"))) {
    switch (c) {
      case 'c':
        snprintf(path, 1024, "%s", optarg);
        path_opt = true;
        break;
      case 'h':
        Usage();
        exit(0);
        return;
      default:
        Usage();
        exit(-1);
    }
  }

  if (path_opt == false) {
    fprintf(stderr, "Please specify the conf file path\n" );
    Usage();
    exit(-1);
  }

  *zgw_conf = new ZgwConf(path);
  if ((*zgw_conf)->LoadConf() != 0) {
    LOG(FATAL) << "zp-meta load conf file error";
  }
  (*zgw_conf)->Dump();
}

static void daemonize() {
  pid_t pid;
  umask(0);

  if ((pid = fork()) < 0) {
    LOG(FATAL) << "can't fork" << std::endl;
    exit(1);
  } else if (pid != 0)
    exit(0);
  setsid();

  // close std fd
  int fd;
  if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
  } else {
    LOG(FATAL) << "close std fd failed" << std::endl;
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    Usage();
    return 1;
  }

  ZgwConf *zgw_conf;
  ZgwConfInit(&zgw_conf, argc, argv);

  GlogInit(zgw_conf);
  ZgwSignalSetup();

 // FileLocker db_lock(g_zgw_conf->lock_file());
 // Status s = db_lock.Lock();
 // if (!s.ok()) {
 //   return 1;
 // }

  if (zgw_conf->daemonize) {
    daemonize();
  }

  LOG(INFO) << "Start Server on " << zgw_conf->server_ip <<
    ": " << zgw_conf->server_port;

  g_zgw_server = new ZgwServer(zgw_conf->zp_meta_ip_port,
                               zgw_conf->server_ip,
                               zgw_conf->server_port);
  g_zgw_server->Start();

  delete g_zgw_server;
  delete zgw_conf;
  ::google::ShutdownGoogleLogging();
  return 0;
}

