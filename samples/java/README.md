# AWS S3 Java SDK Sample

This is a sample that demonstrates how to make basic requests to Zeppelin S3 gateway using the AWS SDK for Java.

## 准备

*   申请Zeppelin S3 Services的AccessKey和SecretKey。
*   准备AWS Java SDK，见[http://aws.amazon.com/sdkforjava](http://aws.amazon.com/sdkforjava)。或直接下载 [here](https://sdk-for-java.amazonwebservices.com/latest/aws-java-sdk.zip).
*   Zeppelin S3 需要SDK目录下`third-party/lib/*`以及`lib/aws-java-sdk-1.11.155.jar`，其余文件可以删除

## 运行Sample

1. 修改`S3Sample.java`中的`getAWSAccessKeyId()`和`getAWSSecretKey()`的返回值以及`s3.setEndpoint(...)`的参数；
2. 运行samples/java目录下`S3Sample.java`

**NOTE:** sample目录下的build.xml是ant的配置文件，注意修改`third-party/lib`和`lib`的目录位置

详细的API用法可见[Java S3 API Documentation](http://docs.aws.amazon.com/AWSJavaSDK/latest/javadoc/com/amazonaws/services/s3/AmazonS3Client.html)，目前Zeppelin gateway支持的API见首页README
