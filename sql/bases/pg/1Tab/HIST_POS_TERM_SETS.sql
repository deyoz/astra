CREATE TABLE HIST_POS_TERM_SETS (
    ADDRESS VARCHAR(100),
    AIRLINE VARCHAR(3),
    AIRP VARCHAR(3),
    CLIENT_ID INTEGER NOT NULL,
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    NAME VARCHAR(10) NOT NULL,
    PR_DENIAL SMALLINT NOT NULL,
    SERIAL VARCHAR(30) NOT NULL,
    SHOP_ID INTEGER NOT NULL,
    VENDOR_ID INTEGER NOT NULL
);
