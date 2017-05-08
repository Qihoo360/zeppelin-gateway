struct User {
  std::string user_id;
  std::string display_name;
  std::map<std::string, std::string> key_pairs;
}

struct Bucket {
 uint64_t create_time;
 std::string owner;
 std::string acl;
 std::string location;
 int64_t volumn;
 int64_t uploading_volumn;
}

struct Object {
  std::string etag;
  int64_t size;
  std::string owner;
  timeval last_modified;
  int8_t storage_class;
  std::string acl;
  std::string upload_id;
  std::string data_block;
}
