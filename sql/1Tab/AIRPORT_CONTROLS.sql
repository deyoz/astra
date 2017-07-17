create table AIRPORT_CONTROLS
(
AIRLINE  number(9) not null,
CPNNUM   number not null,
DATE_CR  date default sysdate not null,
RECLOC   varchar2(6),
TICKNUM  varchar2(13) not null,
CONSTRAINT AIRPORT_CONTROLS_PK PRIMARY KEY (AIRLINE, TICKNUM, CPNNUM)
);
