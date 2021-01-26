CREATE TABLE ARX_POINTS (
    ACT_IN DATE,
    ACT_OUT DATE,
    AIRLINE VARCHAR2(3),
    AIRLINE_FMT NUMBER(1),
    AIRP VARCHAR2(3) NOT NULL,
    AIRP_FMT NUMBER(1) NOT NULL,
    BORT VARCHAR2(10),
    CRAFT VARCHAR2(3),
    CRAFT_FMT NUMBER(1),
    EST_IN DATE,
    EST_OUT DATE,
    FIRST_POINT NUMBER(9),
    FLT_NO NUMBER(5),
    LITERA VARCHAR2(3),
    MOVE_ID NUMBER(9) NOT NULL,
    PARK_IN VARCHAR2(3),
    PARK_OUT VARCHAR2(3),
    PART_KEY DATE NOT NULL,
    POINT_ID NUMBER(9) NOT NULL,
    POINT_NUM NUMBER(5) NOT NULL,
    PR_DEL NUMBER(1) DEFAULT 0 NOT NULL,
    PR_REG NUMBER(1) NOT NULL,
    PR_TRANZIT NUMBER(1) NOT NULL,
    REMARK VARCHAR2(250),
    SCD_IN DATE,
    SCD_OUT DATE,
    SUFFIX VARCHAR2(1),
    SUFFIX_FMT NUMBER(1),
    TID NUMBER(9) NOT NULL,
    TRIP_TYPE VARCHAR2(1)
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
    PARTITION ARX2017 VALUES LESS THAN(TO_DATE(' 2018-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
    PARTITION ARX2018 VALUES LESS THAN(TO_DATE(' 2019-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
    PARTITION ARX2019 VALUES LESS THAN(TO_DATE(' 2020-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN')),
    PARTITION ARX2020 VALUES LESS THAN(TO_DATE(' 2021-01-01 00:00:00', 'SYYYY-MM-DD HH24:MI:SS', 'NLS_CALENDAR=GREGORIAN'))
*/
);
