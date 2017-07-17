create table WC_TICKET
(
AIRLINE  number(9)      not null,
RECLOC   varchar2(6)    not null,
TICKNUM  varchar2(13)   not null,
CONSTRAINT WC_TICKET_PK PRIMARY KEY(AIRLINE, TICKNUM)
);
