CREATE TABLE OLD_DATE_TIME_ZONESPEC (
    COUNTRY VARCHAR2(2),
    DST_ABBR VARCHAR2(5),
    DST_ADJUSTMENT VARCHAR2(9),
    DST_END_DATE_RULE VARCHAR2(10),
    DST_NAME VARCHAR2(50),
    DST_START_DATE_RULE VARCHAR2(10),
    END_TIME VARCHAR2(9),
    GMT_OFFSET VARCHAR2(9),
    ID VARCHAR2(50) NOT NULL,
    PR_DEL NUMBER(1) NOT NULL,
    START_TIME VARCHAR2(9),
    STD_ABBR VARCHAR2(5),
    STD_NAME VARCHAR2(50),
    TID NUMBER(9) NOT NULL
);
