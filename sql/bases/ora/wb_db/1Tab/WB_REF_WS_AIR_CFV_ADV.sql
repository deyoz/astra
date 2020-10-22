CREATE TABLE WB_REF_WS_AIR_CFV_ADV (
CH_BALANCE_ARM NUMBER NOT NULL,
CH_INDEX_UNIT NUMBER NOT NULL,
CH_NON_STANDART NUMBER NOT NULL,
CH_STANDART NUMBER NOT NULL,
CH_USE_BY_DEFAULT NUMBER NOT NULL,
CH_VOLUME NUMBER NOT NULL,
CH_WEIGHT NUMBER NOT NULL,
DATE_FROM DATE DEFAULT sysdate,
DATE_WRITE DATE NOT NULL,
DENSITY NUMBER,
ID NUMBER NOT NULL,
IDN NUMBER NOT NULL,
ID_AC NUMBER NOT NULL,
ID_BORT NUMBER NOT NULL,
ID_WS NUMBER NOT NULL,
MAX_VOLUME NUMBER,
MAX_WEIGHT NUMBER,
PROC_NAME VARCHAR2(200) NOT NULL,
REMARKS CLOB NOT NULL,
U_HOST_NAME VARCHAR2(200) NOT NULL,
U_IP VARCHAR2(200) NOT NULL,
U_NAME VARCHAR2(200) NOT NULL
);
