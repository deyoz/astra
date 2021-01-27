CREATE TABLE HIST_CITIES (
    CODE VARCHAR2(3) NOT NULL,
    CODE_LAT VARCHAR2(3),
    COUNTRY VARCHAR2(2) NOT NULL,
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    ID NUMBER(9) NOT NULL,
    NAME VARCHAR2(50) NOT NULL,
    NAME_LAT VARCHAR2(50),
    PR_DEL NUMBER(1) NOT NULL,
    TZ_REGION VARCHAR2(50)
);
