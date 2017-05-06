/**
 * @api {get} /:bucket_name?list-type=2 GET Bucket (list object version 2)
 * @apiVersion 0.1.0
 * @apiName GET Bucket (list object version 2)
 * @apiGroup Buckets
 * @apiDescription List 1000 Objects at most in bucket.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {Number{0-1000}}  [max-keys] 需要返回的最大数量
 * @apiParam (Url Parameters) {Number{2}} list-type ListObjects的版本号，有的话只能是2
 * @apiParam (Url Parameters) {String} [delimiter] 根据此字符进行前缀分组
 * @apiParam (Url Parameters) {String} [start-after] 以此值列出排序Objects
 * @apiParam (Url Parameters) {String} [prefix] 列出name符合此prefix的Objects
 *
 * @apiSuccess (Response Elements) {String} CommonPrefixes 根据delimiter分组的各组
 * @apiSuccess (Response Elements) {String} IsTruncated Objects列表是否截断
 * @apiSuccess (Response Elements) {String} StartAfter 标记返回的Objects list从哪里开始
 * @apiSuccess (Response Elements) {String} NextContinuationToken 如果返回列表被截断，该字段说明是从哪里截断
 * @apiSuccess (Response Elements) {String} Prefix 返回列表所用的prefix
 * @apiSuccess (Response Elements) {Number} KeyCount 此次返回列表Key的数目
 *
 * @apiParamExample Sample-Request:
 *    GET /bucket HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    Authorization: authorization string
 *    Content-Type: text/plain
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    <?xml version="1.0" encoding="UTF-8"?>
 *    <ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
 *      <Name>bucket</Name>
 *      <Prefix/>
 *      <StartAfter/>
 *      <KeyCount>2</KeyCount>
 *      <MaxKeys>1000</MaxKeys>
 *      <IsTruncated>false</IsTruncated>
 *      <Contents>
 *        <Key>my-image.jpg</Key>
 *        <LastModified>2009-10-12T17:50:30.000Z</LastModified>
 *        <ETag>&quot;fba9dede5f27731c9771645a39863328&quot;</ETag>
 *        <Size>434234</Size>
 *        <StorageClass>STANDARD</StorageClass>
 *        <Owner>
 *          <ID>75aa57f09aa0c8caeab4f8c24e99d10f8e7faeebf76c078efc7c6caea54ba06a</ID>
 *          <DisplayName>mtd@amazon.com</DisplayName>
 *        </Owner>
 *        </Contents>
 *      <Contents>
 *        <Key>my-third-image.jpg</Key>
 *        <LastModified>2009-10-12T17:50:30.000Z</LastModified>
 *        <ETag>&quot;1b2cf535f27731c974343645a3985328&quot;</ETag>
 *        <Size>64994</Size>
 *        <StorageClass>STANDARD_IA</StorageClass>
 *        <Owner>
 *          <ID>75aa57f09aa0c8caeab4f8c24e99d10f8e7faeebf76c078efc7c6caea54ba06a</ID>
 *          <DisplayName>mtd@amazon.com</DisplayName>
 *        </Owner>
 *      </Contents>
 *    </ListBucketResult>
 *
 * @apiErrorExample {xml} Error-Response-NoSuchBucket
 *    HTTP/1.1 404 Not Found
 *    <?xml version='1.0' encoding='utf-8' ?>
 *    <Error>
 *      <Code>NoSuchBucket</Code>
 *      <Message>The specified bucket does not exist.</Message>
 *      <BucketName>bk2</BucketName>
 *      <RequestId>tx00000000000000000113c-0058a43a07-7deaf-sh-bt-1</RequestId>
 *      <HostId7deaf-sh-bt-1-sh/>
 *    </Error>
 */

/**
 * @api {get} /:bucket_name GET Bucket (list object version 1)
 * @apiVersion 0.1.0
 * @apiName GET Bucket (list object version 1)
 * @apiGroup Buckets
 * @apiDescription List 1000 Objects at most in bucket.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {Number{0-1000}}  [max-keys] 需要返回的最大数量
 * @apiParam (Url Parameters) {String} [delimiter] 根据此字符进行前缀分组
 * @apiParam (Url Parameters) {String} [marker] 以此marker列出Objects
 * @apiParam (Url Parameters) {String} [prefix] 列出name符合此prefix的Objects
 *
 * @apiSuccess (Response Elements) {String} CommonPrefixes 根据delimiter分组的各组
 * @apiSuccess (Response Elements) {String} IsTruncated Objects列表是否截断
 * @apiSuccess (Response Elements) {String} Marker 标记返回的Objects list从哪里开始
 * @apiSuccess (Response Elements) {String} NextMarker 如果返回列表被截断，该字段说明是从哪里截断
 * @apiSuccess (Response Elements) {String} Prefix 返回列表所用的prefix
 *
 * @apiParamExample Sample-Request:
 *    GET /bucket HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    Authorization: authorization string
 *    Content-Type: text/plain
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    <?xml version="1.0" encoding="UTF-8"?>
 *    <ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
 *      <Name>bucket</Name>
 *      <Prefix/>
 *      <Marker/>
 *      <MaxKeys>1000</MaxKeys>
 *      <IsTruncated>false</IsTruncated>
 *      <Contents>
 *        <Key>my-image.jpg</Key>
 *        <LastModified>2009-10-12T17:50:30.000Z</LastModified>
 *        <ETag>&quot;fba9dede5f27731c9771645a39863328&quot;</ETag>
 *        <Size>434234</Size>
 *        <StorageClass>STANDARD</StorageClass>
 *        <Owner>
 *          <ID>75aa57f09aa0c8caeab4f8c24e99d10f8e7faeebf76c078efc7c6caea54ba06a</ID>
 *          <DisplayName>mtd@amazon.com</DisplayName>
 *        </Owner>
 *        </Contents>
 *      <Contents>
 *        <Key>my-third-image.jpg</Key>
 *        <LastModified>2009-10-12T17:50:30.000Z</LastModified>
 *        <ETag>&quot;1b2cf535f27731c974343645a3985328&quot;</ETag>
 *        <Size>64994</Size>
 *        <StorageClass>STANDARD_IA</StorageClass>
 *        <Owner>
 *          <ID>75aa57f09aa0c8caeab4f8c24e99d10f8e7faeebf76c078efc7c6caea54ba06a</ID>
 *          <DisplayName>mtd@amazon.com</DisplayName>
 *        </Owner>
 *      </Contents>
 *    </ListBucketResult>
 *
 * @apiErrorExample {xml} Error-Response-NoSuchBucket
 *    HTTP/1.1 404 Not Found
 *    <?xml version='1.0' encoding='utf-8' ?>
 *    <Error>
 *      <Code>NoSuchBucket</Code>
 *      <Message>The specified bucket does not exist.</Message>
 *      <BucketName>bk2</BucketName>
 *      <RequestId>tx00000000000000000113c-0058a43a07-7deaf-sh-bt-1</RequestId>
 *      <HostId7deaf-sh-bt-1-sh/>
 *    </Error>
 */

/**
 * @api {get} /:bucket_name?uploads= List Multipart Uploads
 * @apiVersion 0.1.0
 * @apiName List Multipart Uploads
 * @apiGroup Buckets
 * @apiDescription This operation returns at most 1,000 multipart uploads in the response.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {Number{0-1000}}  [max-uploads] 需要返回的最大数量
 * @apiParam (Url Parameters) {String} [delimiter] 根据此字符进行前缀分组
 * @apiParam (Url Parameters) {String} [key-marker] 以此key开始列出排序multipart upload
 * @apiParam (Url Parameters) {String} [upload-id-marker] 以此uploadId开始列出排序multipart upload
 * @apiParam (Url Parameters) {String} [prefix] 列出name符合此prefix的Objects
 *
 * @apiSuccess (Response Elements) {String} CommonPrefixes 根据delimiter分组的各组
 * @apiSuccess (Response Elements) {String} IsTruncated multipart upload列表是否截断
 * @apiSuccess (Response Elements) {String} NextKeyMarker 如果返回列表被截断，该字段说明是从哪里截断
 * @apiSuccess (Response Elements) {String} NextUploadIdMarker 如果返回列表被截断，该字段说明是从哪里截断
 * @apiSuccess (Response Elements) {String} Prefix 返回列表所用的prefix
 *
 * @apiParamExample Sample-Request:
 *    GET /bucket?uploads=&max-uploads=3 HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    Authorization: authorization string
 *    Content-Type: text/plain
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *
 *    <?xml version="1.0" encoding="UTF-8"?>
 *    <ListMultipartUploadsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
 *      <Bucket>bucket</Bucket>
 *      <KeyMarker></KeyMarker>
 *      <UploadIdMarker></UploadIdMarker>
 *      <NextKeyMarker>my-movie.m2ts</NextKeyMarker>
 *      <NextUploadIdMarker>YW55IGlkZWEgd2h5IGVsdmluZydzIHVwbG9hZCBmYWlsZWQ</NextUploadIdMarker>
 *      <MaxUploads>3</MaxUploads>
 *      <IsTruncated>true</IsTruncated>
 *      <Upload>
 *        <Key>my-divisor</Key>
 *        <UploadId>XMgbGlrZSBlbHZpbmcncyBub3QgaGF2aW5nIG11Y2ggbHVjaw</UploadId>
 *        <Initiator>
 *          <ID>arn:aws:iam::111122223333:user/user1-11111a31-17b5-4fb7-9df5-b111111f13de</ID>
 *          <DisplayName>user1-11111a31-17b5-4fb7-9df5-b111111f13de</DisplayName>
 *        </Initiator>
 *        <Owner>
 *          <ID>75aa57f09aa0c8caeab4f8c24e99d10f8e7faeebf76c078efc7c6caea54ba06a</ID>
 *        <Initiated>2010-11-10T20:48:33.000Z</Initiated>
 *      </Upload>
 *      <Upload>
 *        <Key>my-movie.m2ts</Key>
 *        <UploadId>VXBsb2FkIElEIGZvciBlbHZpbmcncyBteS1tb3ZpZS5tMnRzIHVwbG9hZA</UploadId>
 *        <Initiator>
 *          <ID>b1d16700c70b0b05597d7acd6a3f92be</ID>
 *          <DisplayName>InitiatorDisplayName</DisplayName>
 *        </Initiator>
 *        <Owner>
 *          <ID>b1d16700c70b0b05597d7acd6a3f92be</ID>
 *          <DisplayName>OwnerDisplayName</DisplayName>
 *          </Owner>
 *          <StorageClass>STANDARD</StorageClass>
 *          <Initiated>2010-11-10T20:48:33.000Z</Initiated>
 *      </Upload>
 *      <Upload>
 *        <Key>my-movie.m2ts</Key>
 *        <UploadId>YW55IGlkZWEgd2h5IGVsdmluZydzIHVwbG9hZCBmYWlsZWQ</UploadId>
 *        <Initiator>
 *          <ID>arn:aws:iam::444455556666:user/user1-22222a31-17b5-4fb7-9df5-b222222f13de</ID>
 *          <DisplayName>user1-22222a31-17b5-4fb7-9df5-b222222f13de</DisplayName>
 *        </Initiator>
 *        <Owner>
 *          <ID>b1d16700c70b0b05597d7acd6a3f92be</ID>
 *          <DisplayName>OwnerDisplayName</DisplayName>
 *        </Owner>
 *        <StorageClass>STANDARD</StorageClass>
 *        <Initiated>2010-11-10T20:49:33.000Z</Initiated>
 *      </Upload>
 *    </ListMultipartUploadsResult>
 *
 */
/**
 * @api {head} /:bucket_name HEAD Bucket
 * @apiVersion 0.1.0
 * @apiName HEAD Bucket
 * @apiGroup Buckets
 * @apiDescription Check whether bucket exist.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 *
 * @apiParamExample Sample-Request:
 *    HEAD /myawsbucket HTTP/1.1
 *    Date: Fri, 10 Feb 2012 21:34:55 GMT
 *    Authorization: authorization string
 *    Host: www.sample-host.com
 *    Connection: Keep-Alive
 *    
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    x-amz-id-2: JuKZqmXuiwFeDQxhD7M8KtsKobSzWA1QEjLbTMTagkKdBX2z7Il/jGhDeJ3j6s80
 *    x-amz-request-id: 32FE2CEB32F5EE25
 *    Date: Fri, 10 2012 21:34:56 GMT
 *    Server: AmazonS3
 *
 */

/**
 * @api {delete} /:bucket_name DELETE Bucket
 * @apiVersion 0.1.0
 * @apiName DELETE Bucket
 * @apiGroup Buckets
 * @apiDescription Delte bucket if exist.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 *
 * @apiParamExample Sample-Request:
 *    DELETE /quotes HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 01 Mar 2006 12:00:00 GMT
 *    Authorization: authorization string
 *    
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 204 No Content
 *    x-amz-id-2: JuKZqmXuiwFeDQxhD7M8KtsKobSzWA1QEjLbTMTagkKdBX2z7Il/jGhDeJ3j6s80
 *    x-amz-request-id: 32FE2CEB32F5EE25
 *    Date: Wed, 01 Mar  2006 12:00:00 GMT
 *    Connection: close
 *    Server: AmazonS3
 *
 */

/**
 * @api {put} /:bucket_name PUT Bucket
 * @apiVersion 0.1.0
 * @apiName PUT Bucket
 * @apiGroup Buckets
 * @apiDescription Create bucket if not exist.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 *
 * @apiParamExample Sample-Request:
 *    PUT /mybucket HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 01 Mar 2006 12:00:00 GMT
 *    Authorization: authorization string
 *    
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    x-amz-id-2: JuKZqmXuiwFeDQxhD7M8KtsKobSzWA1QEjLbTMTagkKdBX2z7Il/jGhDeJ3j6s80
 *    x-amz-request-id: 32FE2CEB32F5EE25
 *    Date: Wed, 01 Mar  2006 12:00:00 GMT
 *    Connection: close
 *    Server: AmazonS3
 *
 */
