#!/bin/bash
  # s3tests.functional.test_s3:test_bucket_list_marker_before_list"
  # s3tests.functional.test_s3:test_bucket_list_marker_after_list"
cases="\
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
  s3tests.functional.test_s3:test_bucket_list_marker_not_in_list"

S3_USE_SIGV4=1 S3TEST_CONF=test4zgw.conf virtualenv/bin/nosetests $cases -a \
auth_aws4 -x --debug-log=./tests.log --cover-html --process-timeout=60 \
--processes=10
