CREATE TABLE APPS_PAX_DATA (
APPS_PAX_ID VARCHAR2(15),
ARV_DATE DATE NOT NULL,
ARV_PORT VARCHAR2(3) NOT NULL,
BIRTH_COUNTRY VARCHAR2(3),
CHECK_CHAR VARCHAR2(1),
CICX_MSG_ID NUMBER(9),
CIRQ_MSG_ID NUMBER(9) NOT NULL,
CKIN_FLT_NUM VARCHAR2(8),
CKIN_POINT_ID NUMBER(9),
CKIN_PORT VARCHAR2(3),
DATE_OF_BIRTH VARCHAR2(8),
DEP_DATE DATE NOT NULL,
DEP_PORT VARCHAR2(3) NOT NULL,
DOC_TYPE VARCHAR2(1),
EXPIRY_DATE VARCHAR2(8),
FAMILY_NAME VARCHAR2(40) NOT NULL,
FLT_NUM VARCHAR2(8) NOT NULL,
GIVEN_NAMES VARCHAR2(40),
ISSUING_STATE VARCHAR2(3),
IS_ENDORSEE VARCHAR2(1),
NATIONALITY VARCHAR2(3),
PASSPORT VARCHAR2(14),
PAX_CREW VARCHAR2(1) NOT NULL,
PAX_ID NUMBER(9) NOT NULL,
PNR_LOCATOR VARCHAR2(6),
PNR_SOURCE VARCHAR2(3),
POINT_ID NUMBER(9) NOT NULL,
PRE_CKIN NUMBER(1) NOT NULL,
SEND_TIME DATE NOT NULL,
SEX VARCHAR2(1),
STATUS VARCHAR2(1),
SUP_CHECK_CHAR VARCHAR2(1),
SUP_DOC_TYPE VARCHAR2(1),
SUP_PASSPORT VARCHAR2(14),
TRANSFER_AT_DEST VARCHAR2(1),
TRANSFER_AT_ORGN VARCHAR2(1)
);
