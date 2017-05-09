#include "zgw_store.h"
#include <iostream>

int main() {
  zgwstore::ZgwStore* store;
  std::vector<std::string> zp_addrs = {"127.0.0.1:9221", "127.0.0.1:9222"};
  std::string redis_addr = "127.0.0.1:6379";

  slash::Status s = zgwstore::ZgwStore::Open(zp_addrs, redis_addr,
      "lock_name", 30000, &store);
  std::cout << "Open ret: " << s.ToString() << std::endl;
  
  zgwstore::User user1;
  user1.user_id = "001";
  user1.display_name = "songzhao";
  user1.key_pairs.insert(std::pair<std::string, std::string>("szacc1", "pri1"));
  user1.key_pairs.insert(std::pair<std::string, std::string>("szacc2", "pri2"));
  s = store->AddUser(user1);
  std::cout << "AddUser ret: " << s.ToString() << std::endl;

  zgwstore::User user2;
  user2.user_id = "002";
  user2.display_name = "gaodunqiao";
  user2.key_pairs.insert(std::pair<std::string, std::string>("gdqacc1", "gdqpri1"));
  user2.key_pairs.insert(std::pair<std::string, std::string>("gdqacc2", "gdqpri2"));
  user2.key_pairs.insert(std::pair<std::string, std::string>("gdqacc3", "gdqpri3"));
  s = store->AddUser(user2);
  std::cout << "AddUser ret: " << s.ToString() << std::endl;

  std::vector<zgwstore::User> users;
  s = store->ListUsers(users);
  std::cout << "ListUsers ret: " << s.ToString() << std::endl;
  for (auto& user : users) {
    std::cout << "-------------------------------" << std::endl;
    std::cout << "user_id: " << user.user_id << std::endl;
    std::cout << "display_name: " << user.display_name << std::endl;
    for (auto& iter : user.key_pairs) {
      std::cout << iter.first << ": " << iter.second << std::endl;
    }
  }

  zgwstore::Bucket bucket;
  bucket.bucket_name = "bucket1";
  bucket.create_time = 0;
  bucket.owner = "songzhao";
  bucket.acl = "acl";
  bucket.location = "us-east-1";
  bucket.volumn = 0;
  bucket.uploading_volumn = 0;
  s = store->AddBucket(bucket);
  std::cout << "AddBucket ret: " << s.ToString() << std::endl;

  delete store;
  std::cout << "Bye" << std::endl;
}
