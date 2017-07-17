create table WC_COUPON
(
NUM	       number(1)      not null,
OWNER_AIRLINE  number(9)      not null,
RECLOC         varchar2(6)    not null,
STATUS         number(2)      not null,
TICKNUM        varchar2(13)   not null,
CONSTRAINT WC_COUPON_PK PRIMARY KEY(OWNER_AIRLINE, TICKNUM, NUM)
);
