import boto3
from botocore.client import Config
    
s3_cli = boto3.client('s3', 'sh-bt-1',
    config=Config(signature_version='s3v4'), use_ssl=False,
    # verify='/path-to/cacert.pem, or delete me and set use_ssl=False',
    endpoint_url='http://zeppelin-gateway-host',
    aws_secret_access_key='XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX',
    aws_access_key_id='XXXXXXXXXXXXXXXXXXXX')

bucket_name = 'mybucket-python-example'

response = s3_cli.create_bucket(Bucket = bucket_name)
print response

# Upload file
with open('file', 'rb') as data:
  s3_cli.upload_fileobj(data, bucket_name, 'filedata')

# Upload data
response = s3_cli.put_object(Bucket = bucket_name, Key = 'ob1', Body = 'this is ob1')
print response

# Download file
with open('file_downloaded', 'wb') as data:
  s3_cli.download_fileobj(bucket_name, 'filedata', data)

# Download data
response = s3_cli.get_object(Bucket = bucket_name, Key = 'ob1')
print response

# Clean up
response = s3_cli.delete_objects(Bucket = bucket_name,
    Delete = {
      'Objects': [
          {
            'Key' : 'filedata'
          },
          {
            'Key' : 'ob1'
          }
        ]
      })
print response

response = s3_cli.delete_bucket(Bucket = bucket_name)
print response
