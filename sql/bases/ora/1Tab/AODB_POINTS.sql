CREATE TABLE AODB_POINTS (
    AODB_POINT_ID NUMBER(10),
    OVERLOAD_ALARM NUMBER(1) DEFAULT 0 NOT NULL,
    POINT_ADDR VARCHAR2(6) NOT NULL,
    POINT_ID NUMBER(9) NOT NULL,
    PR_DEL NUMBER(1) NOT NULL,
    REC_NO_BAG NUMBER(6),
    REC_NO_FLT NUMBER(6),
    REC_NO_PAX NUMBER(6),
    REC_NO_UNACCOMP NUMBER(6),
    SCD_OUT_EXT DATE
);
