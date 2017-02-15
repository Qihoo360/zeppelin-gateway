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


//ZgwConf *g_zgw_conf;
ZgwServer* g_zgw_server;

static void GlogInit() {
  //std::string log_path = g_zgw_conf->log_path();
  std::string log_path("./log");
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
          "  ./zgw ip port\n");
}

//void ZgwConfInit(int argc, char* argv[]) {
//  if (argc < 1) {
//    Usage();
//    exit(-1);
//  }
//  bool path_opt = false;
//  char c;
//  char path[1024];
//  while (-1 != (c = getopt(argc, argv, "c:h"))) {
//    switch (c) {
//      case 'c':
//        snprintf(path, 1024, "%s", optarg);
//        path_opt = true;
//        break;
//      case 'h':
//        Usage();
//        exit(-1);
//        return;
//      default:
//        Usage();
//        exit(-1);
//    }
//  }
//
//  if (path_opt == false) {
//    fprintf(stderr, "Please specify the conf file path\n" );
//    Usage();
//    exit(-1);
//  }
//
//  g_zgw_conf = new ZgwConf();
//  if (g_zgw_conf->Load(path) != 0) {
//    LOG(FATAL) << "zp-meta load conf file error";
//  }
//  g_zgw_conf->Dump();
//}

int main(int argc, char** argv) {
  //ZpConfInit(argc, argv);
  //if (g_zgw_conf->daemonize()) {
  //  daemonize();
  //}

  GlogInit();
  ZgwSignalSetup();

 // FileLocker db_lock(g_zgw_conf->lock_file());
 // Status s = db_lock.Lock();
 // if (!s.ok()) {
 //   return 1;
 // }

 // if (g_zgw_conf->daemonize()) {
 //   create_pid_file();
 //   close_std();
 // }

  if (argc < 3) {
    Usage();
    return 1;
  }
  std::string sport(argv[2]);
  long port;
  slash::string2l(sport.data(), sport.size(), &port);
  LOG(INFO) << "Start Server on " << argv[1] << ":" << port;

  g_zgw_server = new ZgwServer(argv[1], port);
  g_zgw_server->Start();

  delete g_zgw_server;
  ::google::ShutdownGoogleLogging();
  return 0;
}

