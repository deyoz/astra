alter table foreign_scan add constraint foreign_scan__client_types__fk foreign key(client_type) references client_types(code);

