CREATE TABLE KIOSK_ALIASES (
    APP_ID INTEGER,
    DESCR VARCHAR(1000),
    GRP_ID INTEGER,
    ID INTEGER NOT NULL,
    KIOSK_ID VARCHAR(100),
    LANG_ID INTEGER NOT NULL,
    NAME VARCHAR(500) NOT NULL,
    PR_DEL SMALLINT NOT NULL,
    TID INTEGER NOT NULL,
    VALUE VARCHAR(2000)
);
