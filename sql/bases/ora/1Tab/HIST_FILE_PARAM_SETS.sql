CREATE TABLE HIST_FILE_PARAM_SETS (
    AIRLINE VARCHAR2(3),
    AIRP VARCHAR2(3),
    FLT_NO NUMBER(5),
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    ID NUMBER(9) NOT NULL,
    OWN_POINT_ADDR VARCHAR2(6) NOT NULL,
    PARAM_NAME VARCHAR2(20) NOT NULL,
    PARAM_VALUE VARCHAR2(1000),
    POINT_ADDR VARCHAR2(6) NOT NULL,
    PR_SEND NUMBER(1) NOT NULL,
    TYPE VARCHAR2(10) NOT NULL
);
