CREATE TABLE CRS_DISPLACE2 (
AIRLINE VARCHAR2(3) NOT NULL,
AIRP_ARV_SPP VARCHAR2(3) NOT NULL,
AIRP_ARV_TLG VARCHAR2(3) NOT NULL,
AIRP_DEP VARCHAR2(3) NOT NULL,
CLASS_SPP VARCHAR2(1) NOT NULL,
CLASS_TLG VARCHAR2(1) NOT NULL,
FLT_NO NUMBER(5) NOT NULL,
POINT_ID_SPP NUMBER(9) NOT NULL,
POINT_ID_TLG NUMBER(9),
SCD DATE NOT NULL,
STATUS VARCHAR2(1) NOT NULL,
SUFFIX VARCHAR2(1)
);