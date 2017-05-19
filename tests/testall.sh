#!/bin/bash
# s3tests.functional.test_s3:test_bucket_list_delimiter_percentage \
# s3tests.functional.test_s3:test_object_raw_put \
# s3tests.functional.test_s3:test_bucket_list_delimiter_whitespace \
cases="\
  s3tests.functional.test_s3:test_bucket_list_empty \
  s3tests.functional.test_s3:test_bucket_list_distinct \
  s3tests.functional.test_s3:test_bucket_list_object_time \
  s3tests.functional.test_s3:test_object_write_to_nonexist_bucket \
  s3tests.functional.test_s3:test_object_read_notexist \
  s3tests.functional.test_s3:test_object_requestid_on_error \
  s3tests.functional.test_s3:test_bucket_notexist \
  s3tests.functional.test_s3:test_bucket_delete_notexist \
  s3tests.functional.test_s3:test_bucket_delete_nonempty \
  s3tests.functional.test_s3:test_bucket_create_delete \
  s3tests.functional.test_s3:test_object_requestid_matchs_header_on_error \
  s3tests.functional.test_s3:test_object_write_file \
  
  s3tests.functional.test_s3:test_multipart_upload_empty \
  s3tests.functional.test_s3:test_multipart_upload_small \
  s3tests.functional.test_s3:test_multipart_upload_contents \
  s3tests.functional.test_s3:test_multipart_upload_overwrite_existing_object \
  s3tests.functional.test_s3:test_abort_multipart_upload \
  s3tests.functional.test_s3:test_abort_multipart_upload_not_found \
  s3tests.functional.test_s3:test_list_multipart_upload \
  s3tests.functional.test_s3:test_multipart_upload_missing_part \
  s3tests.functional.test_s3:test_multipart_upload_incorrect_etag \
  s3tests.functional.test_s3:test_atomic_multipart_upload_write \
  s3tests.functional.test_s3:test_multipart_resend_first_finishes_last \

  s3tests.functional.test_s3:test_bucket_create_naming_good_long_250 \
  s3tests.functional.test_s3:test_bucket_create_naming_good_long_251 \
  s3tests.functional.test_s3:test_bucket_create_naming_good_long_252 \
  s3tests.functional.test_s3:test_bucket_create_naming_good_long_253 \
  s3tests.functional.test_s3:test_bucket_create_naming_good_long_254 \
  s3tests.functional.test_s3:test_bucket_create_naming_good_long_255 \
  s3tests.functional.test_s3:test_bucket_list_long_name \
  s3tests.functional.test_s3:test_bucket_create_exists \
  s3tests.functional.test_s3:test_bucket_configure_recreate \
  s3tests.functional.test_s3:test_bucket_create_exists_nonowner \
  s3tests.functional.test_s3:test_buckets_create_then_list \
  s3tests.functional.test_s3:test_list_buckets_invalid_auth \
  s3tests.functional.test_s3:test_bucket_create_naming_good_starts_alpha \
  s3tests.functional.test_s3:test_bucket_create_naming_good_starts_digit \
  s3tests.functional.test_s3:test_bucket_create_naming_good_contains_period \
  s3tests.functional.test_s3:test_bucket_create_naming_good_contains_hyphen \
  s3tests.functional.test_s3:test_object_copy_zero_size \
  s3tests.functional.test_s3:test_object_copy_same_bucket \
  s3tests.functional.test_s3:test_object_copy_diff_bucket \
  s3tests.functional.test_s3:test_object_copy_not_owned_bucket \
  s3tests.functional.test_s3:test_object_copy_bucket_not_found \
  s3tests.functional.test_s3:test_object_copy_key_not_found \
  s3tests.functional.test_s3:test_atomic_write_bucket_gone \
  s3tests.functional.test_s3:test_bucket_head \

  s3tests.functional.test_s3:test_atomic_read_1mb \
  s3tests.functional.test_s3:test_atomic_read_4mb \
  s3tests.functional.test_s3:test_atomic_read_8mb \
  s3tests.functional.test_s3:test_atomic_write_1mb \
  s3tests.functional.test_s3:test_atomic_write_4mb \
  s3tests.functional.test_s3:test_atomic_write_8mb \
  s3tests.functional.test_s3:test_atomic_dual_write_1mb \
  s3tests.functional.test_s3:test_atomic_dual_write_4mb \
  s3tests.functional.test_s3:test_atomic_dual_write_8mb \
  s3tests.functional.test_s3:test_atomic_conditional_write_1mb \

  s3tests.functional.test_s3:test_bucket_head_extended \
  s3tests.functional.test_s3:test_object_head_zero_bytes \
  s3tests.functional.test_s3:test_object_write_check_etag \
  s3tests.functional.test_s3:test_ranged_request_response_code \
  s3tests.functional.test_s3:test_ranged_request_skip_leading_bytes_response_code \
  s3tests.functional.test_s3:test_ranged_request_return_trailing_bytes_response_code \
  s3tests.functional.test_s3:test_ranged_request_invalid_range \
  s3tests.functional.test_s3:test_bucket_list_delimiter_basic \
  s3tests.functional.test_s3:test_bucket_list_delimiter_alt \
  s3tests.functional.test_s3:test_bucket_list_delimiter_empty \
  s3tests.functional.test_s3:test_bucket_list_delimiter_none \
  s3tests.functional.test_s3:test_bucket_list_delimiter_not_exist \
  s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_prefix_not_exist \
  s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_delimiter_not_exist \
  s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_prefix_delimiter_not_exist \
  s3tests.functional.test_s3:test_bucket_list_maxkeys_zero \
  s3tests.functional.test_s3:test_bucket_list_maxkeys_none \
  s3tests.functional.test_s3:test_bucket_list_maxkeys_invalid \
  s3tests.functional.test_s3:test_bucket_list_marker_none \
  s3tests.functional.test_s3:test_bucket_list_marker_empty \
  s3tests.functional.test_s3:test_bucket_list_marker_not_in_list \
  s3tests.functional.test_s3:test_bucket_list_marker_after_list \
  s3tests.functional.test_s3:test_bucket_list_marker_before_list \
  s3tests.functional.test_s3:test_ranged_request_empty_object \
  s3tests.functional.test_s3:test_bucket_list_delimiter_dot \
  s3tests.functional.test_s3:test_bucket_list_delimiter_prefix \
  s3tests.functional.test_s3:test_bucket_list_prefix_basic \
  s3tests.functional.test_s3:test_bucket_list_prefix_alt \
  s3tests.functional.test_s3:test_bucket_list_prefix_empty \
  s3tests.functional.test_s3:test_bucket_list_prefix_none \
  s3tests.functional.test_s3:test_bucket_list_prefix_not_exist \
  s3tests.functional.test_s3:test_list_buckets_bad_auth \
  s3tests.functional.test_s3:test_multi_object_delete \
  s3tests.functional.test_s3:test_object_write_read_update_read_delete"

S3_USE_SIGV4=1 S3TEST_CONF=test4zgw.conf virtualenv/bin/nosetests $cases -a \
auth_aws4 -x --debug-log=./tests.log --cover-html --process-timeout=60 \
--processes=1
