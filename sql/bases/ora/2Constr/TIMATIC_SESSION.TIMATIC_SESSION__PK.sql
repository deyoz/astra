alter table timatic_session add constraint timatic_session__pk primary key (
    host,
    port,
    username,
    sub_username,
    pwd
);
