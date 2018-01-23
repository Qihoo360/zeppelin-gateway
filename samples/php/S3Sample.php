<?php

require 'aws-autoloader.php';
use Aws\S3\S3Client;
use Aws\Exception\AwsException;

$options = [
  'region'            => 'us-west-2',
  'version'           => '2006-03-01',
  'signature_version' => 'v4',
  'use_path_style_endpoint' => true,
  'endpoint' => 'http://zeppelin-gateway-host',
  'credentials' => [
    'key'    => 'XXXXXXXXXXXXXXXXXXXX',
    'secret' => 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
  ]
];

$testbk = 'mybucket-php-example';
$testkey = 'ob1';

$s3Client = new S3Client($options);

// Create Bucket
$result = $s3Client->createBucket(array('Bucket' => $testbk));
print_r($result);

// Put Object
$result = $s3Client->putObject(array(
  'Bucket' => $testbk,
  'Key' => $testkey,
  'Body' => 'hello'
));
print_r($result);

// Get Object
$result = $s3Client->getObject(array(
  'Bucket' => $testbk,
  'Key' => $testkey
));
echo ($result['Body']);

// Create presign-url
$cmd = $s3Client->getCommand('GetObject', [
  'Bucket' => $testbk,
  'Key' => $testkey
]);

$request = $s3Client->createPresignedRequest($cmd, '+20 minutes');

// Get the actual presigned-url
$presignedUrl = (string)$request->getUri();
print_r($presignedUrl."\n");

// Clean up
$result = $s3Client->deleteObject(array(
  'Bucket' => $testbk,
  'Key' => $testkey
));

$result = $s3Client->deleteBucket(array(
  'Bucket' => $testbk,
));

?>
