CREATE TABLE ARX_CRS_DISPLACE2 (
    AIRLINE VARCHAR(3) NOT NULL,
    AIRP_ARV_SPP VARCHAR(3) NOT NULL,
    AIRP_ARV_TLG VARCHAR(3) NOT NULL,
    AIRP_DEP VARCHAR(3) NOT NULL,
    CLASS_SPP VARCHAR(1) NOT NULL,
    CLASS_TLG VARCHAR(1) NOT NULL,
    FLT_NO INTEGER NOT NULL,
    PART_KEY TIMESTAMP(0) NOT NULL,
    POINT_ID_SPP INTEGER NOT NULL,
    POINT_ID_TLG INTEGER,
    SCD TIMESTAMP(0) NOT NULL,
    STATUS VARCHAR(1) NOT NULL,
    SUFFIX VARCHAR(1)
);

