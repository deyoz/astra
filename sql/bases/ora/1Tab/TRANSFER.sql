CREATE TABLE TRANSFER (
    AIRLINE_FMT NUMBER(1) NOT NULL,
    AIRP_ARV VARCHAR2(3) NOT NULL,
    AIRP_ARV_FMT NUMBER(1) NOT NULL,
    AIRP_DEP_FMT NUMBER(1) NOT NULL,
    GRP_ID NUMBER(9) NOT NULL,
    PIECE_CONCEPT NUMBER(1),
    POINT_ID_TRFER NUMBER(9) NOT NULL,
    PR_FINAL NUMBER(1) NOT NULL,
    SUFFIX_FMT NUMBER(1),
    TRANSFER_NUM NUMBER(1) NOT NULL
);
