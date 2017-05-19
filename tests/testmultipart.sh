#!/bin/bash
cases="\
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
  s3tests.functional.test_s3:test_multipart_resend_first_finishes_last"

S3_USE_SIGV4=1 S3TEST_CONF=test4zgw.conf virtualenv/bin/nosetests $cases -a \
auth_aws4 -x --debug-log=./tests.log --cover-html --process-timeout=60 \
--processes=10
