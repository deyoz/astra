CREATE TABLE HIST_CODESHARE_SETS (
AIRLINE_MARK VARCHAR2(3) NOT NULL,
AIRLINE_OPER VARCHAR2(3) NOT NULL,
AIRP_DEP VARCHAR2(3) NOT NULL,
DAYS VARCHAR2(7),
FIRST_DATE DATE NOT NULL,
FLT_NO_MARK NUMBER(5) NOT NULL,
FLT_NO_OPER NUMBER(5) NOT NULL,
HIST_ORDER NUMBER(9) NOT NULL,
HIST_TIME DATE NOT NULL,
ID NUMBER(9) NOT NULL,
LAST_DATE DATE,
PR_DEL NUMBER(1) NOT NULL,
PR_MARK_BP NUMBER(1) NOT NULL,
PR_MARK_NORMS NUMBER(1) NOT NULL,
PR_MARK_RPT NUMBER(1) NOT NULL,
suffix_oper VARCHAR2(1),
suffix_mark VARCHAR2(1)
);