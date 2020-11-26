CREATE TABLE HIST_CONFIRMATION_SETS (
AIRLINE VARCHAR2(3) NOT NULL,
AIRP_DEP VARCHAR2(3),
BRAND_AIRLINE VARCHAR2(3),
BRAND_CODE VARCHAR2(10),
CLASS VARCHAR2(1),
DCS_ACTION VARCHAR2(20) NOT NULL,
FQT_AIRLINE VARCHAR2(3),
FQT_TIER_LEVEL VARCHAR2(50),
HIST_ORDER NUMBER(9) NOT NULL,
HIST_TIME DATE NOT NULL,
ID NUMBER(9) NOT NULL,
REM_CODE VARCHAR2(4),
RFISC VARCHAR2(15),
SUBCLASS VARCHAR2(1),
TEXT VARCHAR2(250) NOT NULL,
TEXT_LAT VARCHAR2(250) NOT NULL
);