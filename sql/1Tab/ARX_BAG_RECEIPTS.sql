CREATE TABLE ARX_BAG_RECEIPTS (
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
PART_KEY DATE NOT NULL,
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
/*
) PARTITION BY RANGE (PART_KEY) (
PARTITION ARX200610 VALUES LESS THAN(TO_DATE(' 2007-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200701 VALUES LESS THAN(TO_DATE(' 2007-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200704 VALUES LESS THAN(TO_DATE(' 2007-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200707 VALUES LESS THAN(TO_DATE(' 2007-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200710 VALUES LESS THAN(TO_DATE(' 2008-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200801 VALUES LESS THAN(TO_DATE(' 2008-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200804 VALUES LESS THAN(TO_DATE(' 2008-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200807 VALUES LESS THAN(TO_DATE(' 2008-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200810 VALUES LESS THAN(TO_DATE(' 2009-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200901 VALUES LESS THAN(TO_DATE(' 2009-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200904 VALUES LESS THAN(TO_DATE(' 2009-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200907 VALUES LESS THAN(TO_DATE(' 2009-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX200910 VALUES LESS THAN(TO_DATE(' 2010-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201001 VALUES LESS THAN(TO_DATE(' 2010-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201004 VALUES LESS THAN(TO_DATE(' 2010-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201007 VALUES LESS THAN(TO_DATE(' 2010-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201010 VALUES LESS THAN(TO_DATE(' 2011-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201101 VALUES LESS THAN(TO_DATE(' 2011-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201104 VALUES LESS THAN(TO_DATE(' 2011-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201107 VALUES LESS THAN(TO_DATE(' 2011-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201110 VALUES LESS THAN(TO_DATE(' 2012-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201201 VALUES LESS THAN(TO_DATE(' 2012-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201204 VALUES LESS THAN(TO_DATE(' 2012-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201207 VALUES LESS THAN(TO_DATE(' 2012-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201210 VALUES LESS THAN(TO_DATE(' 2013-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201301 VALUES LESS THAN(TO_DATE(' 2013-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201304 VALUES LESS THAN(TO_DATE(' 2013-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201307 VALUES LESS THAN(TO_DATE(' 2013-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201310 VALUES LESS THAN(TO_DATE(' 2014-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201401 VALUES LESS THAN(TO_DATE(' 2014-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201404 VALUES LESS THAN(TO_DATE(' 2014-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201407 VALUES LESS THAN(TO_DATE(' 2014-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201410 VALUES LESS THAN(TO_DATE(' 2015-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201501 VALUES LESS THAN(TO_DATE(' 2015-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201504 VALUES LESS THAN(TO_DATE(' 2015-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201507 VALUES LESS THAN(TO_DATE(' 2015-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201510 VALUES LESS THAN(TO_DATE(' 2016-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201601 VALUES LESS THAN(TO_DATE(' 2016-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201604 VALUES LESS THAN(TO_DATE(' 2016-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201607 VALUES LESS THAN(TO_DATE(' 2016-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201610 VALUES LESS THAN(TO_DATE(' 2017-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201701 VALUES LESS THAN(TO_DATE(' 2017-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201704 VALUES LESS THAN(TO_DATE(' 2017-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201707 VALUES LESS THAN(TO_DATE(' 2017-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201710 VALUES LESS THAN(TO_DATE(' 2018-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'))
*/
);
