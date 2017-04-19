/**
 * @api {get} /admin_list_users GET Users
 * @apiVersion 0.1.0
 * @apiName GET Users
 * @apiGroup Service
 * @apiDescription List User information.
 *
 * @apiSuccessExample Sample-Response:
 *     HTTP/1.1 200 OK
 *     display_name: s3test
 *     abcdeBhpidyJ9vigsaeb
 *     kZRVpHyb78CAua3U3mTrN8TtSbs7yFGela5UKlx2
 *
 */

/**
 * @api {put} /admin_put_user/:name AddUser
 * @apiVersion 0.1.0
 * @apiName AddUser
 * @apiGroup Service
 * @apiDescription Add New User.
 *
 * @apiParam (Url Parameters) {String} name User's Name
 *
 * @apiSuccessExample Sample-Response:
 *     HTTP/1.1 200 OK
 *     abcdeBhpidyJ9vigsaeb
 *     kZRVpHyb78CAua3U3mTrN8TtSbs7yFGela5UKlx2
 *
 */

/**
 * @api {get} / GET Service
 * @apiVersion 0.1.0
 * @apiName GET Service
 * @apiGroup Service
 * @apiDescription List All Buckets.
 *
 * @apiSuccessExample Sample-Response:
 *     HTTP/1.1 200 OK
 *
 *     <?xml version='1.0' encoding='utf-8' ?>
 *     <ListAllMyBucketsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/"
 *       <Owner>
 *         <ID>d5abae9202d32b51156d0f4daa14f10905d29226dc088df1d34416ab9a4ea01c</ID>
 *         <DisplayName>s3test</DisplayName>
 *       </Owner>
 *       <Buckets>
 *         <Bucket>
 *           <Name>bk1</Name>
 *           <CreationDate>2017-03-23T02:59:42.599Z</CreationDate>
 *         </Bucket>
 *         <Bucket>
 *           <Name>bk2</Name>
 *           <CreationDate>2017-03-20T14:45:26.418Z</CreationDate>
 *         </Bucket>
 *       </Buckets>
 *     </ListAllMyBucketsResult>
 */
