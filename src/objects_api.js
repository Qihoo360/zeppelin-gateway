/**
 * @api {get} /:bucket_name/:object_name GET Object
 * @apiVersion 0.1.0
 * @apiName GET Object
 * @apiGroup Objects
 * @apiDescription Get Object
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 *
 * @apiHeader (Request Headers) {String} [range] 获取部分Object
 *
 * @apiParamExample Sample-Request:
 *    GET /bucket/ob1 HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    Authorization: authorization string
 *    Content-Type: text/plain
 *
 * @apiParamExample Sample-Request-Range-Header:
 *    GET /example-bucket/example-object HTTP/1.1
 *    Host: www.sample-host.com
 *    x-amz-date: Fri, 28 Jan 2011 21:32:02 GMT
 *    Range: bytes=0-9
 *    Authorization: AWS AKIAIOSFODNN7EXAMPLE:Yxg83MZaEgh3OZ3l0rLo5RTX11o=
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    x-amz-id-2: eftixk72aD6Ap51TnqcoF8eFidJG9Z/2mkiDFu8yU9AS1ed4OpIszj7UDNEHGran
 *    x-amz-request-id: 318BC8BC148832E5
 *    Date: Mon, 3 Oct 2016 22:32:00 GMT
 *    Last-Modified: Wed, 12 Oct 2009 17:50:00 GMT
 *    ETag: "fba9dede5f27731c9771645a39863328"
 *    Content-Length: 434234
 *    [434234 bytes of object data]
 *
 * @apiSuccessExample Sample-Response-Range-Header:
 *    HTTP/1.1 206 Partial Content
 *    x-amz-id-2: MzRISOwyjmnupCzjI1WC06l5TTAzm7/JypPGXLh0OVFGcJaaO3KW/hRAqKOpIEEp
 *    x-amz-request-id: 47622117804B3E11
 *    Date: Fri, 28 Jan 2011 21:32:09 GMT
 *    x-amz-meta-title: the title
 *    Last-Modified: Fri, 28 Jan 2011 20:10:32 GMT
 *    ETag: "b2419b1e3fd45d596ee22bdf62aaaa2f"
 *    Accept-Ranges: bytes
 *    Content-Range: bytes 0-9/443
 *    Content-Type: text/plain
 *    Content-Length: 10
 *    Server: AmazonS3
 *    [10 bytes of object data]
 */

/**
 * @api {head} /:bucket_name/:object_name HEAD Object
 * @apiVersion 0.1.0
 * @apiName HEAD Object
 * @apiGroup Objects
 * @apiDescription Check whether object exist.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 *
 * @apiParamExample Sample-Request:
 *    HEAD /bk1/ob2 HTTP/1.1
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
 *    ETag: "ac2e33f90b26451d1c8f8987678f860d"
 *    Last-Modified: Tue, 11 Apr 2017 03:22:24 GMT
 *    Server: AmazonS3
 *
 */

/**
 * @api {delete} /:bucket_name/:object_name DELETE Object
 * @apiVersion 0.1.0
 * @apiName DELETE Object
 * @apiGroup Objects
 * @apiDescription Delte object
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 *
 * @apiParamExample Sample-Request:
 *    DELETE /bk1/ob1 HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 01 Mar 2006 12:00:00 GMT
 *    Authorization: authorization string
 *    
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 204 No Content
 *    x-amz-request-id: 32FE2CEB32F5EE25
 *    Date: Wed, 01 Mar  2006 12:00:00 GMT
 *    Connection: close
 *    Server: AmazonS3
 *
 */

/**
 * @api {put} /:bucket_name/:object_name PUT Object
 * @apiVersion 0.1.0
 * @apiName PUT Object
 * @apiGroup Objects
 * @apiDescription Create object.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 *
 * @apiParamExample Sample-Request:
 *    PUT /bk1/ob1 HTTP/1.1
 *    Content-Type: application/x-www-form-urlencoded
 *    Host: www.sample-host.com
 *    X-Amz-Content-Sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
 *    X-Amz-Date: 20170411T024836Z
 *    Authorization: authorization string
 *    Cache-Control: no-cache
 *    Postman-Token: e1428bef-0ef7-072f-0e4b-3b72fb6b37a3
 *    
 *    content
 *    
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    x-amz-request-id: 0A49CE4060975EAC
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    ETag: "9a0364b9e99bb480dd25e1f0284c8555"
 *    Content-Length: 0
 *    Connection: close
 *    Server: AmazonS3
 */

/**
 * @api {put} /:bucket_name/:object_name PUT Object - Copy
 * @apiVersion 0.1.0
 * @apiName PUT Object - Copy
 * @apiGroup Objects
 * @apiDescription Copy exist object.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 *
 * @apiHeader (Request Headers) {String} x-amz-copy-source /source_bucket/source_object
 *
 * @apiParamExample Sample-Request:
 *    PUT /bucket1/my-second-image.jpg HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 28 Oct 2009 22:32:00 GMT
 *    x-amz-copy-source: /bucket/my-image.jpg
 *    Authorization: authorization string
 *    
 * @apiSuccess (Response Elements) {String} CopyObjectResult 目的对象的修改时间及ETag
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    x-amz-request-id: 0A49CE4060975EAC
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    ETag: "9a0364b9e99bb480dd25e1f0284c8555"
 *    Content-Length: 238
 *    Connection: close
 *    Server: AmazonS3
 *
 *    <CopyObjectResult>
 *      <LastModified>2009-10-28T22:32:00</LastModified>
 *      <ETag>"9b2cf535f27731c974343645a3985328"</ETag>
 *    </CopyObjectResult>
 */

/**
 * @api {post} /:bucket_name/:object_name?uploads= Initiate Multipart Upload
 * @apiVersion 0.1.0
 * @apiName Initiate Multipart Upload
 * @apiGroup Objects
 * @apiDescription Initiate multipart upload, return upload ID.
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 *
 * @apiParamExample Sample-Request:
 *    POST /bucket1/my-second-image.jpg?uploads= HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 28 Oct 2009 22:32:00 GMT
 *    Authorization: authorization string
 *    
 * @apiSuccess (Response Elements) {String} Bucket Objects's bucket
 * @apiSuccess (Response Elements) {String} Key Object's key
 * @apiSuccess (Response Elements) {String} UploadId 上传任务的ID
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    Date:  Mon, 1 Nov 2010 20:34:56 GMT
 *    Content-Length: 197
 *    Connection: keep-alive
 *    Server: AmazonS3
 *
 *    <?xml version="1.0" encoding="UTF-8"?>
 *    <InitiateMultipartUploadResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
 *      <Bucket>example-bucket</Bucket>
 *      <Key>example-object</Key>
 *      <UploadId>VXBsb2FkIElEIGZvciA2aWWpbmcncyBteS1tb3ZpZS5tMnRzIHVwbG9hZA</UploadId>
 *    </InitiateMultipartUploadResult>
 */

/**
 * @api {put} /:bucket_name/:object_name?uploadId=:upload_id&partNumber=:part_num Upload Part
 * @apiVersion 0.1.0
 * @apiName Upload Part
 * @apiGroup Objects
 * @apiDescription Upload Part
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 * @apiParam (Url Parameters) {String} upload_id multiupload task's id
 * @apiParam (Url Parameters) {String} part_num multipart's sequence
 *
 * @apiParamExample Sample-Request:
 *    PUT /mybucket/my-movie.m2ts?partNumber=1&uploadId=VCVsb2FkIElEIGZvciBlbZZpbmcncyBteS1tb3ZpZS5tMnRzIHVwbG9hZR HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Mon, 1 Nov 2010 20:34:56 GMT
 *    Content-Length: 10485760
 *    Content-MD5: pUNXr/BjKK5G2UKvaRRrOA==
 *    Authorization: authorization string
 *    
 *    ***part data omitted***
 *    
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    Date:  Mon, 1 Nov 2010 20:34:56 GMT
 *    Content-Length: 197
 *    Connection: keep-alive
 *    ETag: "b54357faf0632cce46e942fa68356b38"
 *    Server: AmazonS3
 */

/**
 * @api {put} /:bucket_name/:object_name?uploadId=:upload_id&partNumber=:part_num Upload Part - Copy
 * @apiVersion 0.1.0
 * @apiName Upload Part - Copy
 * @apiGroup Objects
 * @apiDescription Upload Part - Copy
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 * @apiParam (Url Parameters) {String} upload_id multiupload task's id
 * @apiParam (Url Parameters) {String} part_num multipart's sequence
 *
 * @apiHeader (Request Headers) {String} x-amz-copy-source /source_bucket/source_object
 * @apiHeader (Request Headers) {String} [x-amz-copy-source-range] Copy部分Object
 *
 * @apiParamExample Sample-Request:
 *    PUT /mybucket/my-movie.m2ts?partNumber=1&uploadId=VCVsb2FkIElEIGZvciBlbZZpbmcncyBteS1tb3ZpZS5tMnRzIHVwbG9hZR HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 28 Oct 2009 22:32:00 GMT
 *    x-amz-copy-source: /bucket/my-image.jpg
 *    Authorization: authorization string
 *    
 * @apiSuccess (Response Elements) {String} CopyObjectResult 目的对象的修改时间及ETag
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    x-amz-request-id: 0A49CE4060975EAC
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    ETag: "9a0364b9e99bb480dd25e1f0284c8555"
 *    Content-Length: 238
 *    Connection: close
 *    Server: AmazonS3
 *
 *    <CopyObjectResult>
 *      <LastModified>2009-10-28T22:32:00</LastModified>
 *      <ETag>"9b2cf535f27731c974343645a3985328"</ETag>
 *    </CopyObjectResult>
 */

/**
 * @api {delete} /:bucket_name/:object_name?uploadId=:upload_id Abort Multipart Upload
 * @apiVersion 0.1.0
 * @apiName Abort Multipart Upload
 * @apiGroup Objects
 * @apiDescription Abort Multipart Upload
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 * @apiParam (Url Parameters) {String} upload_id multiupload task's id
 *
 * @apiParamExample Sample-Request:
 *    DELETE /mybucket/my-movie.m2ts?uploadId=VCVsb2FkIElEIGZvciBlbZZpbmcncyBteS1tb3ZpZS5tMnRzIHVwbG9hZR HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 28 Oct 2009 22:32:00 GMT
 *    Authorization: authorization string
 *    
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 204 No Content
 *    x-amz-request-id: 0A49CE4060975EAC
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    Content-Length: 0
 *    Connection: close
 *    Server: AmazonS3
 */

/**
 * @api {post} /:bucket_name/:object_name?uploadId=:upload_id Complete Multipart Upload
 * @apiVersion 0.1.0
 * @apiName Complete Multipart Upload
 * @apiGroup Objects
 * @apiDescription Complete Multipart Upload
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 * @apiParam (Url Parameters) {String} object_name Object's Name
 * @apiParam (Url Parameters) {String} upload_id multiupload task's id
 *
 * @apiParam (Request Elements) {String} CompleteMultipartUpload Container for the request.
 * @apiParam (Request Elements) {String} Part Container for elements related to a particular previously uploaded part.
 * @apiParam (Request Elements) {String} PartNumber Part number that identifies the part.
 * @apiParam (Request Elements) {String} ETag Entity tag returned when the part was uploaded.
 *
 * @apiParamExample Sample-Request:
 *    POST /mybucket/my-movie.m2ts?uploadId=VCVsb2FkIElEIGZvciBlbZZpbmcncyBteS1tb3ZpZS5tMnRzIHVwbG9hZR HTTP/1.1
 *    Host: www.sample-host.com
 *    Date: Wed, 28 Oct 2009 22:32:00 GMT
 *    Authorization: authorization string
 *
 *    <CompleteMultipartUpload>
 *      <Part>
 *        <PartNumber>1</PartNumber>
 *        <ETag>"a54357aff0632cce46d942af68356b38"</ETag>
 *      </Part>
 *      <Part>
 *        <PartNumber>2</PartNumber>
 *        <ETag>"0c78aef83f66abc1fa1e8477f296d394"</ETag>
 *      </Part>
 *      <Part>
 *        <PartNumber>3</PartNumber>
 *        <ETag>"acbd18db4cc2f85cedef654fccc4a4d8"</ETag>
 *      </Part>
 *    </CompleteMultipartUpload>
 *    
 * @apiSuccess (Rsponse Elements) {String} CompleteMultipartUploadResult Container for the response
 * @apiSuccess (Rsponse Elements) {String} ETag 所有Object的ETag的MD5值
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 204 No Content
 *    x-amz-request-id: 0A49CE4060975EAC
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    Content-Length: 0
 *    Connection: close
 *    Server: AmazonS3
 *
 *    <CompleteMultipartUploadResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
 *      <Bucket>Example-Bucket</Bucket>
 *      <Key>Example-Object</Key>
 *      <ETag>"3858f62230ac3c915f300c664312c11f-9"</ETag>
 *    </CompleteMultipartUploadResult>
 */

/**
 * @api {post} /:bucket_name/?delete= Delete Multiple Objects
 * @apiVersion 0.1.0
 * @apiName Delete Multiple Objects
 * @apiGroup Objects
 * @apiDescription Delete Multiple Objects
 *
 * @apiParam (Url Parameters) {String} bucket_name Bucket's Name
 *
 * @apiParam (Request Elements) {String} Delete Container for the request.
 * @apiParam (Request Elements) {String} Key Key name of the object to delete.
 * @apiParam (Request Elements) {String} Object Container element that describes the delete request for an object.
 *
 * @apiParamExample Sample-Request:
 *     POST /?delete HTTP/1.1
 *     Host: bucketname.S3.amazonaws.com
 *     x-amz-date: Wed, 30 Nov 2011 03:39:05 GMT
 *     Content-MD5: p5/WA/oEr30qrEEl21PAqw==
 *     Authorization: AWS AKIAIOSFODNN7EXAMPLE:W0qPYCLe6JwkZAD1ei6hp9XZIee=
 *     Content-Length: 125
 *     Connection: Keep-Alive
 *     <Delete>
 *       <Object>
 *         <Key>sample1.txt</Key>
 *       </Object>
 *       <Object>
 *         <Key>sample2.txt</Key>
 *       </Object>
 *     </Delete>
 *    
 * @apiSuccess (Response Elements) {String} DeleteResult Container for the response
 * @apiSuccess (Response Elements) {String} Deleted 删除成功的Key
 * @apiSuccess (Response Elements) {String} Error 删除失败的Key
 * @apiSuccess (Response Elements) {String} Key 删除成功或者失败的Key
 *
 * @apiSuccessExample Sample-Response:
 *    HTTP/1.1 200 OK
 *    x-amz-request-id: 0A49CE4060975EAC
 *    Date: Wed, 12 Oct 2009 17:50:00 GMT
 *    Content-Length: 0
 *    Connection: close
 *    Server: AmazonS3
 *
 *    <?xml version="1.0" encoding="UTF-8"?>
 *    <DeleteResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
 *      <Deleted>
 *        <Key>sample1.txt</Key>
 *      </Deleted>
 *      <Error>
 *        <Key>sample2.txt</Key>
 *        <Code>AccessDenied</Code>
 *        <Message>Access Denied</Message>
 *      </Error>
 *    </DeleteResult>
 */
