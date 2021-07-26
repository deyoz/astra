CREATE TABLE HIST_CRS_SET (
    AIRLINE VARCHAR(3) NOT NULL,
    AIRP_DEP VARCHAR(3),
    CRS VARCHAR(7) NOT NULL,
    FLT_NO INTEGER,
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    PRIORITY SMALLINT NOT NULL,
    PR_NUMERIC_PNL SMALLINT NOT NULL
);
