#!/bin/bash
# S3_USE_SIGV4=1 S3TEST_CONF=test4zgw.conf virtualenv/bin/nosetests s3tests.functional.test_s3:$1 -a auth_aws4 -v -s
S3_USE_SIGV4=1 S3TEST_CONF=test4zgw.conf virtualenv/bin/nosetests s3tests.functional.test_s3:$1 -a auth_aws4 -v
# S3_USE_SIGV4=1 S3TEST_CONF=test4zgw.conf virtualenv/bin/nosetests $1 -a auth_aws4
