CREATE TABLE APPS_SETS (
AIRLINE VARCHAR2(3) NOT NULL,
APPS_COUNTRY VARCHAR2(2) NOT NULL,
FLT_CLOSEOUT NUMBER(1) NOT NULL,
FORMAT VARCHAR2(10) NOT NULL,
ID NUMBER(9) NOT NULL,
INBOUND NUMBER(1) NOT NULL,
OUTBOUND NUMBER(1) NOT NULL,
PR_DENIAL NUMBER(1) NOT NULL,
PRE_CHECKIN NUMBER(1) DEFAULT 1 NOT NULL
);
