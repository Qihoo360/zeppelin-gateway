#include "src/zgw_admin_conn.h"

#include "src/zgw_util.h"
#include "src/zgw_server.h"

extern ZgwServer* g_zgw_server;

AdminConn::AdminConn(const int fd,
                     const std::string &ip_port,
                     pink::Thread* worker)
      : HttpConn(fd, ip_port, worker) {
	store_ = static_cast<libzgw::ZgwStore*>(worker->get_private());
}

void AdminConn::DealMessage(const pink::HttpRequest* req, pink::HttpResponse* resp) {
  Status s;
  std::string command, params;
  ExtraBucketAndObject(req->path, &command, &params);
  // Users operation, without authorization for now
  if (req->method == "GET") {
    if (command == "admin_list_users") {
      ListUsersHandle(resp);
    } else if (command == "status") {
      ListStatusHandle(resp);
    }
    return;
  } else if (req->method == "PUT" &&
             command == "admin_put_user") {
    if (params.empty()) {
      resp->SetStatusCode(400);
      return;
    }
    std::string access_key;
    std::string secret_key;
    s = store_->AddUser(params, &access_key, &secret_key);
    if (!s.ok()) {
      resp->SetStatusCode(500);
      resp->SetBody(s.ToString());
    } else {
      resp->SetStatusCode(200);
      resp->SetBody(access_key + "\r\n" + secret_key);
    }
    return;
  }
}

void AdminConn::ListStatusHandle(pink::HttpResponse* resp) {
  std::string body;
  std::set<libzgw::ZgwUser *> user_list; // name : keys
  Status s = store_->ListUsers(&user_list);
  if (!s.ok()) {
    resp->SetStatusCode(500);
    resp->SetBody(s.ToString());
  }
  // Buckets qps
  body.append("Global qps: " + std::to_string(g_zgw_server->qps()) + "\r\n");
  // Buckets nums
  std::string access_key;
  for (auto& user : user_list) {
    const auto& info = user->user_info();
    for (auto& key_pair : user->access_keys()) {
      access_key = key_pair.first; // access key
    }
    s = g_zgw_server->RefAndGetBucketList(store_, access_key, &buckets_name_);
    if (!s.ok()) {
      resp->SetStatusCode(500);
      LOG(ERROR) << "ListStatus: list bucket name failed: " << s.ToString();
      return;
    }
    body.append("User: " + info.disply_name + " has "
                + std::to_string(buckets_name_->name_list.size())
                + " Buckets\r\n");
    std::set<std::string> name_list;
    {
      std::lock_guard<std::mutex> lock(buckets_name_->list_lock);
      name_list = buckets_name_->name_list;
    }
    g_zgw_server->UnrefBucketList(store_, access_key);
    for (const auto& name : name_list) {
      s = g_zgw_server->RefAndGetObjectList(store_, name, &objects_name_);
      if (!s.ok()) {
        resp->SetStatusCode(500);
        LOG(ERROR) << "ListStatus: list object name failed: " << s.ToString();
        return;
      }
      body.append("    Bucket: " + name + " has "
                  + std::to_string(objects_name_->name_list.size())
                  + " Objects.\r\n");
      g_zgw_server->UnrefObjectList(store_, name);
    }
  }
  // Bucket space TODO (gaodq)
  resp->SetBody(body);
  resp->SetStatusCode(200);
}

void AdminConn::ListUsersHandle(pink::HttpResponse* resp) {
  std::set<libzgw::ZgwUser *> user_list; // name : keys
  Status s = store_->ListUsers(&user_list);
  LOG(INFO) << "Call ListUsersHandle: " << ip_port();
  if (!s.ok()) {
    resp->SetStatusCode(500);
    resp->SetBody(s.ToString());
  } else {
    resp->SetStatusCode(200);
    std::string body;
    for (auto &user : user_list) {
      const auto &info = user->user_info();
      body.append("disply_name: " + info.disply_name + "\r\n");

      for (auto &key_pair : user->access_keys()) {
        body.append(key_pair.first + "\r\n"); // access key
        body.append(key_pair.second + "\r\n"); // secret key
      }
      body.append("\r\n");
    }
    resp->SetBody(body);
  }
}
