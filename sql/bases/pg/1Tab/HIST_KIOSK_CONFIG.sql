CREATE TABLE HIST_KIOSK_CONFIG (
    APP_ID INTEGER,
    DESCR VARCHAR(1000),
    GRP_ID INTEGER,
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    KIOSK_ID VARCHAR(100),
    NAME VARCHAR(500) NOT NULL,
    PR_DEL SMALLINT NOT NULL,
    VALUE VARCHAR(2000)
);

