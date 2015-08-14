CREATE TABLE ARX_PAX (
BAG_POOL_NUM NUMBER(3),
COUPON_NO NUMBER(1),
DOCO_CONFIRM NUMBER(1),
GRP_ID NUMBER(9) NOT NULL,
IS_FEMALE NUMBER(1),
NAME VARCHAR2(64),
PART_KEY DATE NOT NULL,
PAX_ID NUMBER(9) NOT NULL,
PERS_TYPE VARCHAR2(2) NOT NULL,
PR_BRD NUMBER(1) DEFAULT 0,
PR_EXAM NUMBER(1) NOT NULL,
REFUSE VARCHAR2(1),
REG_NO NUMBER(3) NOT NULL,
SEATS NUMBER(1) DEFAULT 1 NOT NULL,
SEAT_NO VARCHAR2(8),
SEAT_TYPE VARCHAR2(4),
SUBCLASS VARCHAR2(1),
SURNAME VARCHAR2(64) NOT NULL,
TICKET_CONFIRM NUMBER(1) NOT NULL,
TICKET_NO VARCHAR2(15),
TICKET_REM VARCHAR2(5),
TID NUMBER(9) NOT NULL,
WL_TYPE VARCHAR2(1)
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
PARTITION ARX201110 VALUES LESS THAN(TO_DATE(' 2012-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'))
*/
);
