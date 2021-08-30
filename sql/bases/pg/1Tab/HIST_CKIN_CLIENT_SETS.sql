CREATE TABLE HIST_CKIN_CLIENT_SETS (
    AIRLINE VARCHAR(3),
    AIRP_DEP VARCHAR(3),
    CLIENT_TYPE VARCHAR(5) NOT NULL,
    DESK_GRP_ID INTEGER,
    FLT_NO INTEGER,
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    PR_PERMIT SMALLINT NOT NULL,
    PR_TCKIN SMALLINT NOT NULL,
    PR_UPD_STAGE SMALLINT NOT NULL,
    PR_WAITLIST SMALLINT NOT NULL
);

