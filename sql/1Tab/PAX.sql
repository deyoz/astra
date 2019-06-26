CREATE TABLE PAX (
BAG_POOL_NUM NUMBER(3),
CABIN_CLASS VARCHAR2(1),
CABIN_CLASS_GRP NUMBER(9),
CABIN_SUBCLASS VARCHAR2(1),
COUPON_NO NUMBER(1),
CREW_TYPE VARCHAR2(3),
DOCO_CONFIRM NUMBER(1) NOT NULL,
GRP_ID NUMBER(9) NOT NULL,
IS_FEMALE NUMBER(1),
IS_JMP NUMBER(1),
NAME VARCHAR2(64),
PAX_ID NUMBER(9) NOT NULL,
PERS_TYPE VARCHAR2(2) NOT NULL,
PR_BRD NUMBER(1) DEFAULT 0,
PR_EXAM NUMBER(1) NOT NULL,
REFUSE VARCHAR2(1),
REG_NO NUMBER(3) NOT NULL,
SEATS NUMBER(1) DEFAULT 1 NOT NULL,
SEAT_TYPE VARCHAR2(4),
SUBCLASS VARCHAR2(1),
SURNAME VARCHAR2(64) NOT NULL,
TICKET_CONFIRM NUMBER(1) NOT NULL,
TICKET_NO VARCHAR2(15),
TICKET_REM VARCHAR2(5),
TID NUMBER(9) NOT NULL,
WL_TYPE VARCHAR2(1)
);
