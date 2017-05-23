#include "zgw_store.h"
#include <iostream>

int main() {
  zgwstore::ZgwStore* store;
  std::vector<std::string> zp_addrs = {"127.0.0.1:9221", "127.0.0.1:9222"};
  std::string redis_addr = "127.0.0.1:6379";

  slash::Status s = zgwstore::ZgwStore::Open(zp_addrs, redis_addr,
      "s3_1", "lock_name", 30000, "", &store);
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
  s = store->ListUsers(&users);
  std::cout << "ListUsers ret: " << s.ToString() << std::endl;
  for (auto& user : users) {
    std::cout << "-------------------------------" << std::endl;
    std::cout << "user_id: " << user.user_id << std::endl;
    std::cout << "display_name: " << user.display_name << std::endl;
    for (auto& iter : user.key_pairs) {
      std::cout << iter.first << ": " << iter.second << std::endl;
    }
  }

  zgwstore::Bucket bucket1;
  bucket1.bucket_name = "bucket1";
  bucket1.create_time = 0;
  bucket1.owner = "songzhao";
  bucket1.acl = "acl";
  bucket1.location = "us-east-1";
  bucket1.volumn = 0;
  bucket1.uploading_volumn = 0;
  s = store->AddBucket(bucket1);
  std::cout << "AddBucket ret: " << s.ToString() << std::endl;

  zgwstore::Bucket bucket2;
  bucket2.bucket_name = "bucket2";
  bucket2.create_time = 68;
  bucket2.owner = "songzhao";
  bucket2.acl = "acl";
  bucket2.location = "us-east-1";
  bucket2.volumn = 66;
  bucket2.uploading_volumn = 88;
  s = store->AddBucket(bucket2);
  std::cout << "AddBucket ret: " << s.ToString() << std::endl;

  zgwstore::Bucket buc;
  s = store->GetBucket("songzhao", "bucket1", &buc);
  std::cout << "GetBucket ret: " << s.ToString() << std::endl;
  std::cout << "-------------------------------" << std::endl;
  std::cout << "bucket_name: " << buc.bucket_name << std::endl;
  std::cout << "create_time: " << buc.create_time << std::endl;
  std::cout << "owner: " << buc.owner << std::endl;
  std::cout << "acl: " << buc.acl << std::endl;
  std::cout << "location: " << buc.location << std::endl;
  std::cout << "volumn: " << buc.volumn << std::endl;
  std::cout << "uploading_volumn: " << buc.uploading_volumn << std::endl;

  std::vector<zgwstore::Bucket> buckets;
  s = store->ListBuckets("songzhao", &buckets);
  std::cout << "ListBuckets ret: " << s.ToString() << std::endl;
  for (auto& bucket : buckets) {
    std::cout << "-------------------------------" << std::endl;
    std::cout << "bucket_name: " << bucket.bucket_name << std::endl;
    std::cout << "create_time: " << bucket.create_time << std::endl;
    std::cout << "owner: " << bucket.owner << std::endl;
    std::cout << "acl: " << bucket.acl << std::endl;
    std::cout << "location: " << bucket.location << std::endl;
    std::cout << "volumn: " << bucket.volumn << std::endl;
    std::cout << "uploading_volumn: " << bucket.uploading_volumn << std::endl;
  }

  uint64_t tail_id = -1;
  s = store->AllocateId("songzhao", "bucket1", "object1", 16, &tail_id);
  std::cout << "AllocateId ret: " << s.ToString() << ", tail_id: " << tail_id << std::endl;

  zgwstore::Object object1;
  object1.bucket_name = "bucket1";
  object1.object_name = "object1";
  object1.etag = "etag";
  object1.size = 6666;
  object1.owner = "songzhao";
  object1.last_modified = 123;
  object1.storage_class = 0;
  object1.acl = "acl";
  object1.upload_id = "1";
  object1.data_block = "6-8";
  s = store->AddObject(object1);
  std::cout << "AddObject ret: " << s.ToString() << std::endl;

  zgwstore::Object obj;
  s = store->GetObject("songzhao", "bucket1", "object1", &obj);
  std::cout << "GetObject ret: " << s.ToString() << std::endl;
  std::cout << "-------------------------------" << std::endl;
  std::cout << "bucket_name: " << obj.bucket_name << std::endl;
  std::cout << "object_name: " << obj.object_name << std::endl;
  std::cout << "etag: " << obj.etag << std::endl;
  std::cout << "size: " << obj.size << std::endl;
  std::cout << "owner: " << obj.owner << std::endl;
  std::cout << "etag: " << obj.etag << std::endl;
  std::cout << "last_modified: " << obj.last_modified << std::endl;
  std::cout << "storage_class: " << obj.storage_class << std::endl;
  std::cout << "acl: " << obj.acl << std::endl;
  std::cout << "upload_id: " << obj.upload_id << std::endl;
  std::cout << "data_block: " << obj.data_block << std::endl;

  std::vector<zgwstore::Object> objs;
  s = store->ListObjects("songzhao", "bucket1", &objs);
  std::cout << "ListObjects ret: " << s.ToString() << std::endl;
  for (auto& obj : objs) {
    std::cout << "-------------------------------" << std::endl;
    std::cout << "bucket_name: " << obj.bucket_name << std::endl;
    std::cout << "object_name: " << obj.object_name << std::endl;
    std::cout << "etag: " << obj.etag << std::endl;
    std::cout << "size: " << obj.size << std::endl;
    std::cout << "owner: " << obj.owner << std::endl;
    std::cout << "etag: " << obj.etag << std::endl;
    std::cout << "last_modified: " << obj.last_modified << std::endl;
    std::cout << "storage_class: " << obj.storage_class << std::endl;
    std::cout << "acl: " << obj.acl << std::endl;
    std::cout << "upload_id: " << obj.upload_id << std::endl;
    std::cout << "data_block: " << obj.data_block << std::endl;
  }

  s = store->DeleteBucket("songzhao", "bucket1");
  std::cout << "DeleteBucket ret: " << s.ToString() << std::endl;

  s = store->DeleteObject("songzhao", "bucket1", "object1");
  std::cout << "DeleteObject ret: " << s.ToString() << std::endl;

  s = store->DeleteBucket("songzhao", "bucket1");
  std::cout << "DeleteBucket ret: " << s.ToString() << std::endl;

  delete store;
  std::cout << "Bye" << std::endl;
}
