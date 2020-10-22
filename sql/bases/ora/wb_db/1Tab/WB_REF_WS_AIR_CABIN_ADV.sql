CREATE TABLE WB_REF_WS_AIR_CABIN_ADV (
CCL_BALANCE_ARM NUMBER NOT NULL,
CCL_INDEX_UNIT NUMBER NOT NULL,
CD_BALANCE_ARM NUMBER NOT NULL,
CD_INDEX_UNIT NUMBER NOT NULL,
DATE_FROM DATE DEFAULT sysdate ,
DATE_WRITE DATE NOT NULL,
FDL_BALANCE_ARM NUMBER NOT NULL,
FDL_INDEX_UNIT NUMBER NOT NULL,
ID NUMBER NOT NULL,
IDN NUMBER NOT NULL,
ID_AC NUMBER NOT NULL,
ID_BORT NUMBER NOT NULL,
ID_WS NUMBER NOT NULL,
U_HOST_NAME VARCHAR2(200) NOT NULL,
U_IP VARCHAR2(200) NOT NULL,
U_NAME VARCHAR2(200) NOT NULL
);
