CREATE TABLE ANNUL_BAG (
    AMOUNT SMALLINT,
    BAG_TYPE SMALLINT,
    GRP_ID INTEGER NOT NULL,
    ID INTEGER NOT NULL,
    PAX_ID INTEGER,
    POINT_ID INTEGER,
    RFISC VARCHAR(15),
    SCD_OUT TIMESTAMP(0),
    TIME_ANNUL TIMESTAMP(0),
    TIME_CREATE TIMESTAMP(0),
    USER_ID INTEGER,
    WEIGHT SMALLINT
);
