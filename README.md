# Zeppelin-gateway

[![Build Status](https://travis-ci.org/Qihoo360/zeppelin-gateway.svg?branch=master)](https://travis-ci.org/Qihoo360/zeppelin-gateway)

Object Gateway Provide Applications with a RESTful Gateway to zeppelin

[API Documents](https://qihoo360.github.io/zeppelin-gateway/)

### SDK Samples
These samples demonstrate how to make basic requests to Zeppelin S3 gateway using the AWS SDK.

[Java](https://github.com/Qihoo360/zeppelin-gateway/tree/master/samples/java)

[Python](https://github.com/Qihoo360/zeppelin-gateway/tree/master/samples/python)

[PHP](https://github.com/Qihoo360/zeppelin-gateway/tree/master/samples/php)

### FEATURES SUPPORT

| Feature                   | Status         |
| :------------------------ | :------------- |
| List Buckets              | Supported      |
| Delete Bucket             | Supported  |
| Create Bucket             | Supported      |
| Policy (Buckets, Objects) | Not Supported  |
| Bucket ACLs (Get, Put)    | Not Supported  |
| Get Bucket Info (HEAD)    | Supported  |
| Get Object                | Supported      |
| Put Object                | Supported      |
| Delete Object             | Supported      |
| Object ACLs (Get, Put)    | Not Supported  |
| Get Object Info (HEAD)    | Supported  |
| POST Object               | Not  Supported |
| Copy Object               | Supported  |
| Multipart Uploads         | Supported  |
