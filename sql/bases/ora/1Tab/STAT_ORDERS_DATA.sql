CREATE TABLE STAT_ORDERS_DATA (
    DOWNLOAD_TIMES NUMBER(9) NOT NULL,
    FILE_ID NUMBER(9) NOT NULL,
    FILE_NAME VARCHAR2(100) NOT NULL,
    FILE_SIZE NUMBER(10),
    FILE_SIZE_ZIP NUMBER(10),
    MD5_SUM VARCHAR2(32),
    MONTH DATE NOT NULL
);
