CREATE TABLE ARX_ANNUL_BAG (
AMOUNT NUMBER(3),
BAG_TYPE NUMBER(2),
GRP_ID NUMBER(9) NOT NULL,
ID NUMBER(9),
PART_KEY DATE NOT NULL,
PAX_ID NUMBER(9),
RFISC VARCHAR2(15),
TIME_ANNUL DATE,
TIME_CREATE DATE,
USER_ID NUMBER(9),
WEIGHT NUMBER(4)
/*
) PARTITION BY RANGE (PART_KEY) (
PARTITION ARX2006 VALUES LESS THAN(TO_DATE(' 2007-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2007 VALUES LESS THAN(TO_DATE(' 2008-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2008 VALUES LESS THAN(TO_DATE(' 2009-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2009 VALUES LESS THAN(TO_DATE(' 2010-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2010 VALUES LESS THAN(TO_DATE(' 2011-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2011 VALUES LESS THAN(TO_DATE(' 2012-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2012 VALUES LESS THAN(TO_DATE(' 2013-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2013 VALUES LESS THAN(TO_DATE(' 2014-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2014 VALUES LESS THAN(TO_DATE(' 2015-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2015 VALUES LESS THAN(TO_DATE(' 2016-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2016 VALUES LESS THAN(TO_DATE(' 2017-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
PARTITION ARX2017 VALUES LESS THAN(TO_DATE(' 2018-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'))
*/
);
