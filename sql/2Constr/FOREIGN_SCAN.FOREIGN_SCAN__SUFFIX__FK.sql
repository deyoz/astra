alter table foreign_scan add constraint foreign_scan__suffix__fk foreign key(suffix) references trip_suffixes(code);

