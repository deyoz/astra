CREATE TABLE PRN_TAG_PROPS (
    CODE VARCHAR2(20) NOT NULL,
    CONVERT_CHAR_VIEW NUMBER(1) NOT NULL,
    EXCEPT_WHEN_GREAT_LEN NUMBER(1) NOT NULL,
    EXCEPT_WHEN_ONLY_LAT NUMBER(1) NOT NULL,
    LENGTH NUMBER(2) NOT NULL,
    OP_TYPE VARCHAR2(10) NOT NULL
);
