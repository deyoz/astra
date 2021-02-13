CREATE TABLE FOREIGN_SCAN (
    AIRLINE VARCHAR2(3) NOT NULL,
    AIRP_ARV VARCHAR2(3) NOT NULL,
    AIRP_DEP VARCHAR2(3) NOT NULL,
    CLIENT_TYPE VARCHAR2(5) NOT NULL,
    DESCR VARCHAR2(100) NOT NULL,
    DESK VARCHAR2(6) NOT NULL,
    DESK_AIRP VARCHAR2(3),
    ERRORS VARCHAR2(200),
    FLT_NO NUMBER(5) NOT NULL,
    ID NUMBER(9) NOT NULL,
    SCAN_DATA VARCHAR2(4000) NOT NULL,
    SCD_OUT DATE NOT NULL,
    SUFFIX VARCHAR2(1),
    TIME_PRINT DATE NOT NULL
);
