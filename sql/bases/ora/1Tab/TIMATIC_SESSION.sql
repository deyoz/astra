create table timatic_session (
    host varchar2(200) not null,
    port number(5) not null,
    username varchar2(200) not null,
    sub_username varchar2(200) not null,
    pwd varchar2(200) not null,
    expire date not null,
    jsessionID varchar2(200),
    sessionID varchar2(200)
);
