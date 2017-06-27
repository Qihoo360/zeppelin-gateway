# AWS S3 Python SDK Sample

This is a sample that demonstrates how to make basic requests to Zeppelin S3 gateway using the AWS SDK for Python.

## 准备

*   申请Zeppelin S3 Services的AccessKey和SecretKey。
*   准备AWS Python boto3 module，`pip install boto3`

## 运行Sample

1. 修改`S3Sample.py`中的`boto3.client(...)` 相关参数：`endpoint_url`，`aws_secret_access_key`，`aws_access_key_id`
2. 运行samples/python目录下`S3Sample.py`

详细的API用法可见[Python S3 API Reference](https://boto3.readthedocs.io/en/latest/reference/services/s3.html#client)，目前Zeppelin gateway支持的API见首页README