CREATE TABLE RFISC_LIST_ITEMS (
    AIRLINE VARCHAR2(3) NOT NULL,
    CATEGORY NUMBER(3) NOT NULL,
    DESCR1 VARCHAR2(2),
    DESCR2 VARCHAR2(2),
    EMD_TYPE VARCHAR2(6),
    GRP VARCHAR2(3),
    LIST_ID NUMBER(9) NOT NULL,
    NAME VARCHAR2(100) NOT NULL,
    NAME_LAT VARCHAR2(100) NOT NULL,
    RFIC VARCHAR2(1),
    RFISC VARCHAR2(15) NOT NULL,
    SERVICE_TYPE VARCHAR2(1) NOT NULL,
    SUBGRP VARCHAR2(3),
    VISIBLE NUMBER(1) NOT NULL
);
