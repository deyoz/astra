CREATE TABLE TLG_TRIPS (
AIRLINE VARCHAR2(3) NOT NULL,
AIRP_ARV VARCHAR2(3),
AIRP_DEP VARCHAR2(3),
BIND_TYPE NUMBER(1) NOT NULL,
FLT_NO NUMBER(5) NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
PR_UTC NUMBER(1) NOT NULL,
SCD DATE NOT NULL,
SUFFIX VARCHAR2(1)
);