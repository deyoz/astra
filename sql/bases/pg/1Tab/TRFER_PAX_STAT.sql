CREATE TABLE TRFER_PAX_STAT (
    BAG_AMOUNT INTEGER,
    BAG_WEIGHT INTEGER,
    PAX_ID INTEGER NOT NULL,
    POINT_ID INTEGER NOT NULL,
    RK_WEIGHT INTEGER,
    SCD_OUT TIMESTAMP(0) NOT NULL,
    SEGMENTS VARCHAR(4000) NOT NULL
);
