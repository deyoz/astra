CREATE TABLE CRAFTS (
    CODE VARCHAR2(3) NOT NULL,
    CODE_ICAO VARCHAR2(4),
    CODE_ICAO_LAT VARCHAR2(4),
    CODE_LAT VARCHAR2(3),
    ID NUMBER(9) NOT NULL,
    NAME VARCHAR2(50) NOT NULL,
    NAME_LAT VARCHAR2(50),
    PR_DEL NUMBER(1) NOT NULL,
    TID NUMBER(9) NOT NULL,
    TID_SYNC NUMBER(9)
);
