#ifndef ZGW_DEFINE_H_
#define ZGW_DEFINE_H_
namespace zgwstore {

const std::string kZpBlockPrefix = "_ZGW_B_";
const std::string kZpRefPrefix = "_ZGW_R_";

const std::string kZgwDeletedList = "#ZDL#";
const std::string kZgwIdGen = "#ZID#";

const std::string kZgwUserList = "#ZUL#";
const std::string kZgwUserPrefix = "_ZU_";

const std::string kZgwBucketListPrefix = "_ZBL_";
const std::string kZgwBucketPrefix = "_ZB_";

const std::string kZgwObjectListPrefix = "_ZOL_";
const std::string kZgwObjectPrefix = "_ZO_";
const std::string kZgwTempObjectNamePrefix = "_@_";

struct User {
  std::string user_id;
  std::string display_name;
  std::map<std::string, std::string> key_pairs;
};

struct Bucket {
 std::string bucket_name;
 uint64_t create_time;
 std::string owner;
 std::string acl;
 std::string location;
 int64_t volumn;
 int64_t uploading_volumn;
};

struct Object {
  std::string bucket_name;
  std::string object_name;
  std::string etag;
  int64_t size;
  std::string owner;
  uint64_t last_modified;
  int32_t storage_class;
  std::string acl;
  std::string upload_id;
  std::string data_block;
};

}
#endif

