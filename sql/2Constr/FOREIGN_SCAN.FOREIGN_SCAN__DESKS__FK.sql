alter table foreign_scan add constraint foreign_scan__desks__fk foreign key(desk) references desks(code);

