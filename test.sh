make source /srvr/z5224815/env
pgs start
dropdb test
createdb test
psql test -f intset.sql
psql test -f test.sql

