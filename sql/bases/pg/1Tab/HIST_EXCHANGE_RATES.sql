CREATE TABLE HIST_EXCHANGE_RATES (
    AIRLINE VARCHAR(3),
    CUR1 VARCHAR(3) NOT NULL,
    CUR2 VARCHAR(3) NOT NULL,
    EXTRA VARCHAR(2000),
    FIRST_DATE TIMESTAMP(0) NOT NULL,
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    LAST_DATE TIMESTAMP(0),
    PR_DEL SMALLINT NOT NULL,
    RATE1 INTEGER NOT NULL,
    RATE2 DECIMAL(10,4) NOT NULL
);

