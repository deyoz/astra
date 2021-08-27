CREATE TABLE HIST_REM_TXT_SETS (
    AIRLINE VARCHAR(3) NOT NULL,
    BRAND_AIRLINE VARCHAR(3),
    BRAND_CODE VARCHAR(10),
    FQT_AIRLINE VARCHAR(3),
    FQT_TIER_LEVEL VARCHAR(50),
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    PR_LAT SMALLINT NOT NULL,
    RFISC VARCHAR(15),
    TAG_INDEX SMALLINT NOT NULL,
    TEXT VARCHAR(100) NOT NULL,
    TEXT_LENGTH SMALLINT NOT NULL
);

