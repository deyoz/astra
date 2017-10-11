CREATE TABLE HIST_EXCHANGE_RATES (
AIRLINE VARCHAR2(3),
CUR1 VARCHAR2(3) NOT NULL,
CUR2 VARCHAR2(3) NOT NULL,
EXTRA VARCHAR2(2000),
FIRST_DATE DATE NOT NULL,
HIST_ORDER NUMBER(9) NOT NULL,
HIST_TIME DATE NOT NULL,
ID NUMBER(9) NOT NULL,
LAST_DATE DATE,
PR_DEL NUMBER(1) NOT NULL,
RATE1 NUMBER(6) NOT NULL,
RATE2 NUMBER(10) NOT NULL
);
