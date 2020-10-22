CREATE TABLE CRS_PAX (
BAG_NORM NUMBER(3),
BAG_NORM_UNIT VARCHAR2(2),
BAG_POOL NUMBER(3),
CABIN_CLASS VARCHAR2(1),
ETICK_CLASS VARCHAR2(1),
ETICK_SUBCLASS VARCHAR2(1),
INF_ID NUMBER(9),
LAST_OP DATE NOT NULL,
NAME VARCHAR2(64),
ORIG_CLASS VARCHAR2(1),
ORIG_SUBCLASS VARCHAR2(1),
PAX_ID NUMBER(9) NOT NULL,
PERS_TYPE VARCHAR2(2) NOT NULL,
PNR_ID NUMBER(9) NOT NULL,
PR_DEL NUMBER(1) DEFAULT 0 NOT NULL,
SEATS NUMBER(1) DEFAULT 1 NOT NULL,
SEAT_REM VARCHAR2(4),
SEAT_TYPE VARCHAR2(4),
SEAT_XNAME VARCHAR2(1),
SEAT_YNAME VARCHAR2(3),
SURNAME VARCHAR2(64) NOT NULL,
SYNC_CHKD NUMBER(1) NOT NULL,
TID NUMBER(9) NOT NULL,
UNIQUE_REFERENCE VARCHAR2(25)
);