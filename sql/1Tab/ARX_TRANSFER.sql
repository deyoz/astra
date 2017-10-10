CREATE TABLE ARX_TRANSFER (
AIRLINE VARCHAR2(3) NOT NULL,
AIRLINE_FMT NUMBER(1) NOT NULL,
AIRP_ARV VARCHAR2(3) NOT NULL,
AIRP_ARV_FMT NUMBER(1) NOT NULL,
AIRP_DEP VARCHAR2(3) NOT NULL,
AIRP_DEP_FMT NUMBER(1) NOT NULL,
FLT_NO NUMBER(5) NOT NULL,
GRP_ID NUMBER(9) NOT NULL,
PART_KEY DATE NOT NULL,
PIECE_CONCEPT NUMBER(1),
PR_FINAL NUMBER(1) NOT NULL,
SCD DATE NOT NULL,
SUFFIX VARCHAR2(1),
SUFFIX_FMT NUMBER(1),
TRANSFER_NUM NUMBER(1) NOT NULL
);
