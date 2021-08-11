CREATE TABLE HIST_TYPEB_ADDRS (
    ADDR VARCHAR(100) NOT NULL,
    AIRLINE VARCHAR(3),
    AIRP_ARV VARCHAR(3),
    AIRP_DEP VARCHAR(3),
    CRS VARCHAR(7),
    FLT_NO INTEGER,
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    PR_LAT SMALLINT NOT NULL,
    PR_MARK_FLT SMALLINT NOT NULL,
    PR_MARK_HEADER SMALLINT NOT NULL,
    TLG_TYPE VARCHAR(6) NOT NULL
);
