> forked from [ceph/s3-tests](https://github.com/ceph/s3-tests)

Run Tests:

Add your IP of host in `test4zgw.conf` to `/etc/hosts`

Then:

```bash
./bootstrap
source virtualenv/bin/activate
./test4zgw.sh
```

To gather a list of tests being run, use the flags:

```bash
./virtualenv/bin/nosetests -v --collect-only
```

You can specify what test(s) to run:

```bash
./test.sh s3tests.functional.test_s3.test_bucket_list_empty
```
