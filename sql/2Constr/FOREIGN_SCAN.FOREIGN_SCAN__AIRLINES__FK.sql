alter table foreign_scan add constraint foreign_scan__airlines__fk foreign key(airline) references airlines(code);

