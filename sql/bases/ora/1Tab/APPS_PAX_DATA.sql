CREATE TABLE APPS_PAX_DATA (
APPS_PAX_ID VARCHAR2(15),
ARV_DATE DATE NOT NULL,
ARV_PORT VARCHAR2(3) NOT NULL,
BIRTH_COUNTRY VARCHAR2(3),
CHECK_CHAR VARCHAR2(1),
CICX_MSG_ID NUMBER(9),
CIRQ_MSG_ID NUMBER(9) NOT NULL,
CITY VARCHAR2(60),
CKIN_FLT_NUM VARCHAR2(8),
CKIN_POINT_ID NUMBER(9),
CKIN_PORT VARCHAR2(3),
COUNTRY_FOR_DATA VARCHAR2(2),
COUNTRY_ISSUANCE VARCHAR2(3),
DATE_OF_BIRTH VARCHAR2(8),
DEP_DATE DATE NOT NULL,
DEP_PORT VARCHAR2(3) NOT NULL,
DOCO_EXPIRY_DATE VARCHAR2(8),
DOCO_NO VARCHAR2(20),
DOCO_TYPE VARCHAR2(2),
DOC_SUBTYPE VARCHAR2(1),
DOC_TYPE VARCHAR2(1),
EXPIRY_DATE VARCHAR2(8),
FAMILY_NAME VARCHAR2(40) NOT NULL,
FLT_NUM VARCHAR2(8) NOT NULL,
GIVEN_NAMES VARCHAR2(40),
ISSUING_STATE VARCHAR2(3),
IS_ENDORSEE VARCHAR2(1),
NATIONALITY VARCHAR2(3),
NUM_STREET VARCHAR2(60),
PASSPORT VARCHAR2(14),
PASS_REF VARCHAR2(5),
PAX_CREW VARCHAR2(1) NOT NULL,
PAX_ID NUMBER(9) NOT NULL,
PNR_LOCATOR VARCHAR2(6),
PNR_SOURCE VARCHAR2(3),
POINT_ID NUMBER(9) NOT NULL,
POSTAL_CODE VARCHAR2(20),
PRE_CKIN NUMBER(1) NOT NULL,
REDRESS_NUMBER VARCHAR2(13),
SEND_TIME DATE NOT NULL,
SEX VARCHAR2(1),
STATE VARCHAR2(20),
STATUS VARCHAR2(1),
SUP_CHECK_CHAR VARCHAR2(1),
SUP_DOC_TYPE VARCHAR2(1),
SUP_PASSPORT VARCHAR2(14),
TRANSFER_AT_DEST VARCHAR2(1),
TRANSFER_AT_ORGN VARCHAR2(1),
TRAVELLER_NUMBER VARCHAR2(25),
VERSION NUMBER(2)
);