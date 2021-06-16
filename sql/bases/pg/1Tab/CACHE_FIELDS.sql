CREATE TABLE CACHE_FIELDS (
    ALIGN VARCHAR(1),
    CHAR_CASE VARCHAR(1),
    CODE VARCHAR(30) NOT NULL,
    DATA_SIZE SMALLINT NOT NULL,
    DATA_TYPE VARCHAR(3) NOT NULL,
    LANG VARCHAR(2),
    NAME VARCHAR(30) NOT NULL,
    NULLABLE SMALLINT NOT NULL,
    NUM SMALLINT NOT NULL,
    PR_IDENT SMALLINT NOT NULL,
    READ_ONLY SMALLINT NOT NULL,
    REFER_CODE VARCHAR(30),
    REFER_IDENT SMALLINT,
    REFER_LEVEL SMALLINT,
    REFER_NAME VARCHAR(30),
    SCALE SMALLINT,
    TITLE VARCHAR(100),
    WIDTH SMALLINT
);

