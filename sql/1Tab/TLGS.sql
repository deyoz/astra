CREATE TABLE TLGS (
ERROR VARCHAR2(4),
ID NUMBER(9) NOT NULL,
POSTPONED NUMBER(1),
RECEIVER VARCHAR2(5) NOT NULL,
SENDER VARCHAR2(5) NOT NULL,
TIME DATE NOT NULL,
TLG_NUM NUMBER(10) NOT NULL,
TYPE VARCHAR2(4) NOT NULL,
TYPEB_TLG_ID NUMBER(9),
TYPEB_TLG_NUM NUMBER(5)
);
