#!/bin/bash

cases="\
  s3tests.functional.test_s3:test_atomic_read_1mb \
  s3tests.functional.test_s3:test_atomic_read_4mb \
  s3tests.functional.test_s3:test_atomic_read_8mb \
  s3tests.functional.test_s3:test_atomic_write_1mb \
  s3tests.functional.test_s3:test_atomic_write_4mb \
  s3tests.functional.test_s3:test_atomic_write_8mb \
  s3tests.functional.test_s3:test_atomic_dual_write_1mb \
  s3tests.functional.test_s3:test_atomic_dual_write_4mb \
  s3tests.functional.test_s3:test_atomic_dual_write_8mb \
  s3tests.functional.test_s3:test_atomic_conditional_write_1mb"

S3_USE_SIGV4=1 S3TEST_CONF=test4zgw.conf virtualenv/bin/nosetests $cases -a \
auth_aws4 -x --debug-log=./tests.log --cover-html --process-timeout=60 \
--processes=10
