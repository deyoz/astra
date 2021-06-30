CREATE TABLE FILE_PARAM_SETS (
    AIRLINE VARCHAR(3),
    AIRP VARCHAR(3),
    FLT_NO INTEGER,
    ID INTEGER NOT NULL,
    OWN_POINT_ADDR VARCHAR(6) NOT NULL,
    PARAM_NAME VARCHAR(20) NOT NULL,
    PARAM_VALUE VARCHAR(1000),
    POINT_ADDR VARCHAR(6) NOT NULL,
    PR_SEND SMALLINT NOT NULL,
    TYPE VARCHAR(10) NOT NULL
);

