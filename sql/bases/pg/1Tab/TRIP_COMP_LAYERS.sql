CREATE TABLE TRIP_COMP_LAYERS (
    CRS_PAX_ID INTEGER,
    FIRST_XNAME VARCHAR(4) NOT NULL,
    FIRST_YNAME VARCHAR(4) NOT NULL,
    LAST_XNAME VARCHAR(4) NOT NULL,
    LAST_YNAME VARCHAR(4) NOT NULL,
    LAYER_TYPE VARCHAR(10) NOT NULL,
    PAX_ID INTEGER,
    POINT_ARV INTEGER,
    POINT_DEP INTEGER,
    POINT_ID INTEGER NOT NULL,
    RANGE_ID INTEGER NOT NULL,
    TIME_CREATE TIMESTAMP(0) NOT NULL
);
