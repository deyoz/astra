CREATE TABLE DCS_ADDR_SET (
AIRIMP_ADDR VARCHAR2(8),
AIRIMP_OWN_ADDR VARCHAR2(8),
AIRLINE VARCHAR2(3) NOT NULL,
EDIFACT_PROFILE VARCHAR2(30),
EDI_ADDR VARCHAR2(20) NOT NULL,
EDI_ADDR_EXT VARCHAR2(20),
EDI_OWN_ADDR VARCHAR2(20) NOT NULL,
EDI_OWN_ADDR_EXT VARCHAR2(20),
FLT_NO NUMBER(5),
ID NUMBER(9) NOT NULL,
OWN_AIRLINE VARCHAR2(3),
OWN_FLT_NO NUMBER(5)
);
