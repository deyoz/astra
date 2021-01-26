CREATE TABLE STAT_PARAMS (
    CAPTION VARCHAR2(20) NOT NULL,
    CODE VARCHAR2(20) NOT NULL,
    CTYPE VARCHAR2(20) NOT NULL,
    EDIT_FMT VARCHAR2(100),
    FILTER VARCHAR2(200),
    ISALNUM NUMBER(1) NOT NULL,
    LEN NUMBER(5) NOT NULL,
    REF VARCHAR2(20),
    REF_FIELD VARCHAR2(20),
    TAG VARCHAR2(20),
    VISIBLE NUMBER(9) NOT NULL,
    WIDTH NUMBER(5) NOT NULL
);
