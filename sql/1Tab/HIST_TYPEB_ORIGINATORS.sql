CREATE TABLE HIST_TYPEB_ORIGINATORS (
ADDR VARCHAR2(7) NOT NULL,
AIRLINE VARCHAR2(3),
AIRP_DEP VARCHAR2(3),
DESCR VARCHAR2(50) NOT NULL,
DOUBLE_SIGN VARCHAR2(2),
FIRST_DATE DATE NOT NULL,
HIST_ORDER NUMBER(9) NOT NULL,
HIST_TIME DATE NOT NULL,
ID NUMBER(9) NOT NULL,
LAST_DATE DATE,
PR_DEL NUMBER(1) NOT NULL,
TLG_TYPE VARCHAR2(6)
);
