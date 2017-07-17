create table WC_PNR
(
AIRLINE  number(9) 	not null,
PAGE_NO  number(3)   	not null,
RECLOC   varchar2(6) 	not null,
TLG_TEXT varchar2(1000) not null,
TLG_TYPE number(1)	not null,
CONSTRAINT WC_PNR_PK PRIMARY KEY(RECLOC)
);
