CREATE TABLE BAG_RECEIPTS (
AIRCODE VARCHAR2(3) NOT NULL,
AIRLINE VARCHAR2(3) NOT NULL,
AIRP_ARV VARCHAR2(3) NOT NULL,
AIRP_DEP VARCHAR2(3) NOT NULL,
ANNUL_DATE DATE,
ANNUL_DESK VARCHAR2(6),
ANNUL_USER_ID NUMBER(9),
BAG_NAME VARCHAR2(50),
BAG_TYPE NUMBER(2),
DESK_LANG VARCHAR2(2) NOT NULL,
EXCH_PAY_RATE NUMBER(10),
EXCH_RATE NUMBER(6),
EX_AMOUNT NUMBER(3),
EX_WEIGHT NUMBER(4),
FLT_NO NUMBER(5),
FORM_TYPE VARCHAR2(7) NOT NULL,
GRP_ID NUMBER(9),
ISSUE_DATE DATE NOT NULL,
ISSUE_DESK VARCHAR2(6) NOT NULL,
ISSUE_PLACE VARCHAR2(100) NOT NULL,
ISSUE_USER_ID NUMBER(9) NOT NULL,
IS_INTER NUMBER(1) NOT NULL,
KIT_ID NUMBER(9),
KIT_NUM NUMBER(1),
NO NUMBER(15) NOT NULL,
PAX_DOC VARCHAR2(50),
PAX_NAME VARCHAR2(50),
PAY_RATE_CUR VARCHAR2(3) NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
PREV_NO VARCHAR2(50),
RATE NUMBER(12) NOT NULL,
RATE_CUR VARCHAR2(3) NOT NULL,
RECEIPT_ID NUMBER(9) NOT NULL,
REMARKS VARCHAR2(50),
SCD_LOCAL_DATE DATE,
SERVICE_TYPE NUMBER(2) NOT NULL,
STATUS VARCHAR2(1) NOT NULL,
SUFFIX VARCHAR2(1),
TICKETS VARCHAR2(50) NOT NULL,
VALUE_TAX NUMBER(4)
);
