CREATE TABLE ARX_BAG_TAGS (
BAG_NUM NUMBER(3),
COLOR VARCHAR2(2),
GRP_ID NUMBER(9) NOT NULL,
NO NUMBER(15) NOT NULL,
NUM NUMBER(3) NOT NULL,
PART_KEY DATE NOT NULL,
PR_PRINT NUMBER(1) NOT NULL,
TAG_TYPE VARCHAR2(5) NOT NULL
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
