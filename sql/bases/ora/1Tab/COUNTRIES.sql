CREATE TABLE COUNTRIES (
    CODE VARCHAR2(2) NOT NULL,
    CODE_ISO VARCHAR2(3),
    CODE_LAT VARCHAR2(2),
    ID NUMBER(9) NOT NULL,
    NAME VARCHAR2(50) NOT NULL,
    NAME_LAT VARCHAR2(50),
    PR_DEL NUMBER(1) NOT NULL,
    TID NUMBER(9) NOT NULL,
    TID_SYNC NUMBER(9)
);
