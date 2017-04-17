/etc/hosts
`hosts: 127.0.0.1               s3.sh-bt-1.s3testhost.com`

test4zgw.conf

### candidate test cases：

- [x] s3tests.functional.test_s3:test_bucket_list_empty

- [x] s3tests.functional.test_s3:test_bucket_list_distinct

- [x] s3tests.functional.test_s3:test_bucket_list_object_time

- [x] s3tests.functional.test_s3:test_object_write_to_nonexist_bucket

- [x] s3tests.functional.test_s3:test_object_read_notexist

- [x] s3tests.functional.test_s3:test_object_requestid_on_error

- [x] s3tests.functional.test_s3:test_bucket_notexist

- [x] s3tests.functional.test_s3:test_bucket_delete_notexist

- [x] s3tests.functional.test_s3:test_bucket_delete_nonempty

- [x] s3tests.functional.test_s3:test_bucket_create_delete

- [x] s3tests.functional.test_s3:test_object_requestid_matchs_header_on_error \*\*

- [x] s3tests.functional.test_s3:test_object_head_zero_bytes

- [x] s3tests.functional.test_s3:test_object_write_check_etag

- [x] s3tests.functional.test_s3:test_object_write_read_update_read_delete

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_none_to_good

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_none_to_empty

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_overwrite_to_good

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_overwrite_to_empty

- [ ] s3tests.functional.test_s3:test_object_set_get_unicode_metadata

- [ ] s3tests.functional.test_s3:test_object_set_get_non_utf8_metadata

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_empty_to_unreadable_prefix

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_empty_to_unreadable_suffix

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_empty_to_unreadable_infix

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_overwrite_to_unreadable_prefix

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_overwrite_to_unreadable_suffix

- [ ] s3tests.functional.test_s3:test_object_set_get_metadata_overwrite_to_unreadable_infix

- [ ] s3tests.functional.test_s3:test_object_metadata_replaced_on_put

- [x] s3tests.functional.test_s3:test_object_write_file

- [ ] s3tests.functional.test_s3:test_get_object_ifmatch_good

- [ ] s3tests.functional.test_s3:test_get_object_ifmatch_failed

- [ ] s3tests.functional.test_s3:test_get_object_ifnonematch_good

- [ ] s3tests.functional.test_s3:test_get_object_ifnonematch_failed

- [ ] s3tests.functional.test_s3:test_get_object_ifmodifiedsince_good

- [ ] s3tests.functional.test_s3:test_get_object_ifmodifiedsince_failed

- [ ] s3tests.functional.test_s3:test_get_object_ifunmodifiedsince_good

- [ ] s3tests.functional.test_s3:test_get_object_ifunmodifiedsince_failed

- [ ] s3tests.functional.test_s3:test_put_object_ifmatch_good

- [ ] s3tests.functional.test_s3:test_put_object_ifmatch_failed

- [ ] s3tests.functional.test_s3:test_put_object_ifmatch_overwrite_existed_good

- [ ] s3tests.functional.test_s3:test_put_object_ifmatch_nonexisted_failed

- [ ] s3tests.functional.test_s3:test_put_object_ifnonmatch_good

- [ ] s3tests.functional.test_s3:test_put_object_ifnonmatch_failed

- [ ] s3tests.functional.test_s3:test_put_object_ifnonmatch_nonexisted_good

- [ ] s3tests.functional.test_s3:test_put_object_ifnonmatch_overwrite_existed_failed

- [ ] s3tests.functional.test_s3:test_object_raw_get

- [ ] s3tests.functional.test_s3:test_object_raw_get_bucket_gone

- [ ] s3tests.functional.test_s3:test_object_delete_key_bucket_gone

- [ ] s3tests.functional.test_s3:test_object_raw_get_object_gone

- [x] s3tests.functional.test_s3:test_bucket_head

- [x] s3tests.functional.test_s3:test_bucket_head_extended

- [ ] s3tests.functional.test_s3:test_object_raw_authenticated

- [ ] s3tests.functional.test_s3:test_object_raw_response_headers

- [ ] s3tests.functional.test_s3:test_object_raw_authenticated_bucket_gone

- [ ] s3tests.functional.test_s3:test_object_raw_authenticated_object_gone

- [ ] s3tests.functional.test_s3:test_object_raw_get_x_amz_expires_not_expired

- [ ] s3tests.functional.test_s3:test_object_raw_get_x_amz_expires_out_range_zero

- [ ] s3tests.functional.test_s3:test_object_raw_get_x_amz_expires_out_max_range

- [ ] s3tests.functional.test_s3:test_object_raw_get_x_amz_expires_out_positive_range

- [x] s3tests.functional.test_s3:test_object_raw_put

- [ ] s3tests.functional.test_s3:test_object_raw_put_write_access

- [ ] s3tests.functional.test_s3:test_object_raw_put_authenticated

- [ ] s3tests.functional.test_s3:test_object_raw_put_authenticated_expired

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_bad_starts_nonalpha

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_bad_short_empty

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_bad_short_one

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_bad_short_two

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_bad_long

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_long_250

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_long_251

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_long_252

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_long_253

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_long_254

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_long_255

- [x] s3tests.functional.test_s3:test_bucket_list_long_name

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_bad_ip

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_bad_punctuation

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_dns_underscore

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_dns_long

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_dns_dash_at_end

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_dns_dot_dot

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_dns_dot_dash

- [ ] s3tests.functional.test_s3:test_bucket_create_naming_dns_dash_dot

- [x] s3tests.functional.test_s3:test_bucket_create_exists

- [x] s3tests.functional.test_s3:test_bucket_configure_recreate

- [ ] s3tests.functional.test_s3:test_bucket_get_location \*

- [x] s3tests.functional.test_s3:test_bucket_create_exists_nonowner \*\*

- [ ] s3tests.functional.test_s3:test_bucket_delete_nonowner

- [x] s3tests.functional.test_s3:test_buckets_create_then_list

- [ ] s3tests.functional.test_s3:test_list_buckets_anonymous

- [x] s3tests.functional.test_s3:test_list_buckets_invalid_auth

- [ ] s3tests.functional.test_s3:test_list_buckets_bad_auth

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_starts_alpha

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_starts_digit

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_contains_period

- [x] s3tests.functional.test_s3:test_bucket_create_naming_good_contains_hyphen

- [ ] s3tests.functional.test_s3:test_bucket_recreate_not_overriding

- [ ] s3tests.functional.test_s3:test_bucket_create_special_key_names \*\*

- [x] s3tests.functional.test_s3:test_bucket_list_special_prefix

- [x] s3tests.functional.test_s3:test_object_copy_zero_size

- [x] s3tests.functional.test_s3:test_object_copy_same_bucket

- [ ] s3tests.functional.test_s3:test_object_copy_verify_contenttype

- [ ] s3tests.functional.test_s3:test_object_copy_to_itself

- [ ] s3tests.functional.test_s3:test_object_copy_to_itself_with_metadata

- [x] s3tests.functional.test_s3:test_object_copy_diff_bucket

- [x] s3tests.functional.test_s3:test_object_copy_not_owned_bucket

- [ ] s3tests.functional.test_s3:test_object_copy_not_owned_object_bucket

- [ ] s3tests.functional.test_s3:test_object_copy_retaining_metadata

- [ ] s3tests.functional.test_s3:test_object_copy_replacing_metadata

- [x] s3tests.functional.test_s3:test_object_copy_bucket_not_found

- [x] s3tests.functional.test_s3:test_object_copy_key_not_found

- [ ] s3tests.functional.test_s3:test_object_copy_versioned_bucket

- [ ] s3tests.functional.test_s3:test_object_copy_versioning_multipart_upload

- [x] s3tests.functional.test_s3:test_multipart_upload_empty

- [ ] s3tests.functional.test_s3:test_multipart_upload_small

- [ ] s3tests.functional.test_s3:test_multipart_copy_small

- [ ] s3tests.functional.test_s3:test_multipart_upload

- [ ] s3tests.functional.test_s3:test_multipart_copy_special_names

- [ ] s3tests.functional.test_s3:test_multipart_copy_versioned

- [ ] s3tests.functional.test_s3:test_multipart_upload_resend_part

- [x] s3tests.functional.test_s3:test_multipart_upload_multiple_sizes

- [ ] s3tests.functional.test_s3:test_multipart_copy_multiple_sizes

- [ ] s3tests.functional.test_s3:test_multipart_upload_size_too_small

- [x] s3tests.functional.test_s3:test_multipart_upload_contents

- [x] s3tests.functional.test_s3:test_multipart_upload_overwrite_existing_object

- [x] s3tests.functional.test_s3:test_abort_multipart_upload

- [x] s3tests.functional.test_s3:test_abort_multipart_upload_not_found

- [x] s3tests.functional.test_s3:test_list_multipart_upload

- [x] s3tests.functional.test_s3:test_multipart_upload_missing_part

- [x] s3tests.functional.test_s3:test_multipart_upload_incorrect_etag

- [ ] s3tests.functional.test_s3:test_100_continue

- [ ] s3tests.functional.test_s3:test_set_cors

- [ ] s3tests.functional.test_s3:test_cors_origin_response

- [ ] s3tests.functional.test_s3:test_cors_origin_wildcard

- [x] s3tests.functional.test_s3:test_atomic_read_1mb

- [x] s3tests.functional.test_s3:test_atomic_read_4mb

- [x] s3tests.functional.test_s3:test_atomic_read_8mb

- [x] s3tests.functional.test_s3:test_atomic_write_1mb

- [x] s3tests.functional.test_s3:test_atomic_write_4mb

- [x] s3tests.functional.test_s3:test_atomic_write_8mb

- [x] s3tests.functional.test_s3:test_atomic_dual_write_1mb

- [x] s3tests.functional.test_s3:test_atomic_dual_write_4mb

- [x] s3tests.functional.test_s3:test_atomic_dual_write_8mb

- [x] s3tests.functional.test_s3:test_atomic_conditional_write_1mb

- [ ] s3tests.functional.test_s3:test_atomic_dual_conditional_write_1mb

- [x] s3tests.functional.test_s3:test_atomic_write_bucket_gone

- [x] s3tests.functional.test_s3:test_atomic_multipart_upload_write

- [x] s3tests.functional.test_s3:test_multipart_resend_first_finishes_last

- [x] s3tests.functional.test_s3:test_ranged_request_response_code

- [x] s3tests.functional.test_s3:test_ranged_request_skip_leading_bytes_response_code

- [x] s3tests.functional.test_s3:test_ranged_request_return_trailing_bytes_response_code

- [x] s3tests.functional.test_s3:test_ranged_request_invalid_range

- [x] s3tests.functional.test_s3:test_ranged_request_empty_object

- [ ] s3tests.functional.test_s3:check_can_test_multiregion

- [ ] s3tests.functional.test_s3:test_region_bucket_create_secondary_access_remove_master

- [ ] s3tests.functional.test_s3:test_region_bucket_create_master_access_remove_secondary

- [ ] s3tests.functional.test_s3:test_region_copy_object

- [ ] s3tests.functional.test_s3:test_versioning_bucket_create_suspend

- [ ] s3tests.functional.test_s3:test_versioning_obj_create_read_remove

- [ ] s3tests.functional.test_s3:test_versioning_obj_create_read_remove_head

- [ ] s3tests.functional.test_s3:test_versioning_obj_plain_null_version_removal

- [ ] s3tests.functional.test_s3:test_versioning_obj_plain_null_version_overwrite

- [ ] s3tests.functional.test_s3:test_versioning_obj_plain_null_version_overwrite_suspended

- [ ] s3tests.functional.test_s3:test_versioning_obj_suspend_versions

- [ ] s3tests.functional.test_s3:test_versioning_obj_suspend_versions_simple

- [ ] s3tests.functional.test_s3:test_versioning_obj_create_versions_remove_all

- [ ] s3tests.functional.test_s3:test_versioning_obj_create_versions_remove_special_names

- [ ] s3tests.functional.test_s3:test_versioning_obj_create_overwrite_multipart

- [ ] s3tests.functional.test_s3:test_versioning_obj_list_marker

- [ ] s3tests.functional.test_s3:test_versioning_copy_obj_version

- [ ] s3tests.functional.test_s3:test_versioning_multi_object_delete

- [ ] s3tests.functional.test_s3:test_versioning_multi_object_delete_with_marker

- [ ] s3tests.functional.test_s3:test_versioning_multi_object_delete_with_marker_create

- [ ] s3tests.functional.test_s3:test_versioned_concurrent_object_create_concurrent_remove

- [ ] s3tests.functional.test_s3:test_versioned_concurrent_object_create_and_remove

- [ ] s3tests.functional.test_s3:test_lifecycle_set

- [ ] s3tests.functional.test_s3:test_lifecycle_get

- [ ] s3tests.functional.test_s3:test_lifecycle_expiration

- [ ] s3tests.functional.test_s3:test_lifecycle_id_too_long

- [ ] s3tests.functional.test_s3:test_lifecycle_same_id

- [ ] s3tests.functional.test_s3:test_lifecycle_invalid_status

- [ ] s3tests.functional.test_s3:test_lifecycle_rules_conflicted

- [ ] s3tests.functional.test_utils:test_generate

- [ ] s3tests.functional.test_s3:test_object_write_expires

- [ ] s3tests.functional.test_s3:test_object_write_cache_control

- [ ] s3tests.functional.test_s3:test_object_create_unreadable

- [ ] s3tests.functional.test_s3:test_multi_object_delete

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_basic

- [ ] s3tests.functional.test_s3:test_bucket_list_delimiter_prefix

- [ ] s3tests.functional.test_s3:test_bucket_list_delimiter_prefix_ends_with_delimiter

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_alt

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_percentage

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_whitespace

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_dot

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_unreadable

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_empty

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_none

- [x] s3tests.functional.test_s3:test_bucket_list_delimiter_not_exist

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_basic

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_alt

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_empty

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_none

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_not_exist

- [ ] s3tests.functional.test_s3:test_bucket_list_prefix_unreadable

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_basic

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_alt

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_prefix_not_exist

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_delimiter_not_exist

- [x] s3tests.functional.test_s3:test_bucket_list_prefix_delimiter_prefix_delimiter_not_exist

- [x] s3tests.functional.test_s3:test_bucket_list_maxkeys_one ??

- [x] s3tests.functional.test_s3:test_bucket_list_maxkeys_zero

- [x] s3tests.functional.test_s3:test_bucket_list_maxkeys_none

- [x] s3tests.functional.test_s3:test_bucket_list_maxkeys_invalid

- [ ] s3tests.functional.test_s3:test_bucket_list_maxkeys_unreadable

- [x] s3tests.functional.test_s3:test_bucket_list_marker_none

- [x] s3tests.functional.test_s3:test_bucket_list_marker_empty

- [ ] s3tests.functional.test_s3:test_bucket_list_marker_unreadable

- [x] s3tests.functional.test_s3:test_bucket_list_marker_not_in_list

- [x] s3tests.functional.test_s3:test_bucket_list_marker_after_list

- [x] s3tests.functional.test_s3:test_bucket_list_marker_before_list

### acl:

- [ ] s3tests.functional.test_s3:test_bucket_list_objects_anonymous

- [ ] s3tests.functional.test_s3:test_bucket_list_objects_anonymous_fail

- [ ] s3tests.functional.test_s3:test_bucket_list_return_data

- [ ] s3tests.functional.test_s3:test_logging_toggle

- [ ] s3tests.functional.test_s3:test_access_bucket_private_object_private

- [ ] s3tests.functional.test_s3:test_access_bucket_private_object_publicread

- [ ] s3tests.functional.test_s3:test_access_bucket_private_object_publicreadwrite

- [ ] s3tests.functional.test_s3:test_access_bucket_publicread_object_private

- [ ] s3tests.functional.test_s3:test_access_bucket_publicread_object_publicread

- [ ] s3tests.functional.test_s3:test_access_bucket_publicread_object_publicreadwrite

- [ ] s3tests.functional.test_s3:test_access_bucket_publicreadwrite_object_private

- [ ] s3tests.functional.test_s3:test_access_bucket_publicreadwrite_object_publicread

- [ ] s3tests.functional.test_s3:test_access_bucket_publicreadwrite_object_publicreadwrite

- [ ] s3tests.functional.test_s3:test_object_giveaway

