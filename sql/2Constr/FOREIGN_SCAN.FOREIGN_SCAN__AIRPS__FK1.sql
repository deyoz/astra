alter table foreign_scan add constraint foreign_scan__airps__fk1 foreign key(airp_dep) references airps(code);

