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
