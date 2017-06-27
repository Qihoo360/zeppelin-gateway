# AWS S3 PHPSDK Sample

This is a sample that demonstrates how to make basic requests to Zeppelin S3 gateway using the AWS SDK for PHP.

## 准备

*   申请Zeppelin S3 Services的AccessKey和SecretKey。
*   准备AWS PHP SDK，见[安装方法](http://docs.aws.amazon.com/aws-sdk-php/v3/guide/getting-started/installation.html)。或直接下载 [zip包](http://docs.aws.amazon.com/aws-sdk-php/v3/download/aws.zip).
*   在php文件中包含SDK：`require '/path/to/aws-autoloader.php';`

## 运行Sample

1. 修改`S3Sample.php`中的相关配置，`endpoint`和`credentials`
2. 运行samples/php目录下的`S3Sample.php`

详细的API用法可见[PHP S3 API Reference](http://docs.aws.amazon.com/aws-sdk-php/v3/api/api-s3-2006-03-01.html)，目前Zeppelin gateway支持的API见首页README