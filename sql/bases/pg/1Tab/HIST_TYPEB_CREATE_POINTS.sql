CREATE TABLE HIST_TYPEB_CREATE_POINTS (
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    STAGE_ID SMALLINT NOT NULL,
    TIME_OFFSET SMALLINT,
    TYPEB_ADDRS_ID INTEGER NOT NULL
);
