CREATE TABLE CODESHARE_SETS (
AIRLINE_MARK VARCHAR2(3) NOT NULL,
AIRLINE_OPER VARCHAR2(3) NOT NULL,
AIRP_DEP VARCHAR2(3) NOT NULL,
DAYS VARCHAR2(7),
FIRST_DATE DATE NOT NULL,
FLT_NO_MARK NUMBER(5) NOT NULL,
FLT_NO_OPER NUMBER(5) NOT NULL,
ID NUMBER(9) NOT NULL,
LAST_DATE DATE,
PR_DEL NUMBER(1) NOT NULL,
PR_MARK_BP NUMBER(1) NOT NULL,
PR_MARK_NORMS NUMBER(1) NOT NULL,
PR_MARK_RPT NUMBER(1) NOT NULL,
SUFFIX_MARK VARCHAR2(1),
SUFFIX_OPER VARCHAR2(1),
TID NUMBER(9) NOT NULL
);
