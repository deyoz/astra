CREATE TABLE ARX_TRANSFER (
    AIRLINE VARCHAR(3) NOT NULL,
    AIRLINE_FMT SMALLINT NOT NULL,
    AIRP_ARV VARCHAR(3) NOT NULL,
    AIRP_ARV_FMT SMALLINT NOT NULL,
    AIRP_DEP VARCHAR(3) NOT NULL,
    AIRP_DEP_FMT SMALLINT NOT NULL,
    FLT_NO INTEGER NOT NULL,
    GRP_ID INTEGER NOT NULL,
    PART_KEY TIMESTAMP(0) NOT NULL,
    PIECE_CONCEPT SMALLINT,
    PR_FINAL SMALLINT NOT NULL,
    SCD TIMESTAMP(0) NOT NULL,
    SUFFIX VARCHAR(1),
    SUFFIX_FMT SMALLINT,
    TRANSFER_NUM SMALLINT NOT NULL
);

