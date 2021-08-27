CREATE TABLE HIST_RFISC_RATES_SELF_CKIN (
    AIRLINE VARCHAR(3) NOT NULL,
    AIRP_ARV VARCHAR(3),
    AIRP_DEP VARCHAR(3),
    CRAFT VARCHAR(3),
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    RATE DECIMAL(12,2) NOT NULL,
    RATE_CUR VARCHAR(3) NOT NULL,
    RFISC VARCHAR(15) NOT NULL
);

