CREATE TABLE TLG_TRFER_ONWARDS (
    AIRLINE VARCHAR(3) NOT NULL,
    AIRP_ARV VARCHAR(3) NOT NULL,
    AIRP_DEP VARCHAR(3) NOT NULL,
    FLT_NO BIGINT NOT NULL,
    GRP_ID INTEGER NOT NULL,
    LOCAL_DATE TIMESTAMP(0) NOT NULL,
    NUM SMALLINT NOT NULL,
    SUBCLASS VARCHAR(1),
    SUFFIX VARCHAR(20)
);

