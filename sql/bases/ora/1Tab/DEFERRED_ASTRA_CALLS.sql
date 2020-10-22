create table DEFERRED_ASTRA_CALLS
(
ID          number(9) not null,
XML_REQ     clob not null,
XML_RES     clob,
TIME_CREATE timestamp not null,
TIME_HANDLE timestamp,
SOURCE      varchar2(30) not null,
STATUS      number(1) not null
);
