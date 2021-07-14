CREATE TABLE AODB_POINTS (
    AODB_POINT_ID BIGINT,
    OVERLOAD_ALARM SMALLINT DEFAULT 0 NOT NULL,
    POINT_ADDR VARCHAR(6) NOT NULL,
    POINT_ID INTEGER NOT NULL,
    PR_DEL SMALLINT NOT NULL,
    REC_NO_BAG INTEGER,
    REC_NO_FLT INTEGER,
    REC_NO_PAX INTEGER,
    REC_NO_UNACCOMP INTEGER,
    SCD_OUT_EXT TIMESTAMP(0)
);
