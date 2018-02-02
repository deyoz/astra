CREATE TABLE ARX_TRFER_PAX_STAT (
BAG_AMOUNT NUMBER(9),
BAG_WEIGHT NUMBER(9),
PART_KEY DATE NOT NULL,
PAX_ID NUMBER(9),
POINT_ID NUMBER(9) NOT NULL,
RK_WEIGHT NUMBER(9),
SCD_OUT DATE NOT NULL,
SEGMENTS VARCHAR2(4000) NOT NULL
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
PARTITION ARX201710 VALUES LESS THAN(TO_DATE(' 2018-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201801 VALUES LESS THAN(TO_DATE(' 2018-04-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201804 VALUES LESS THAN(TO_DATE(' 2018-07-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201807 VALUES LESS THAN(TO_DATE(' 2018-10-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX201810 VALUES LESS THAN(TO_DATE(' 2019-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'))
*/
);
