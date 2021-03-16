create table mySets (id integer primary key, iset intSet);
insert into mySets values (1, '{1,2,3}');
insert into mySets values (2, '{1,3,1,3,1}');
insert into mySets values (3, '{3,4,5}');
insert into mySets values (4, '{4,5}');
select * from mySets order by id;