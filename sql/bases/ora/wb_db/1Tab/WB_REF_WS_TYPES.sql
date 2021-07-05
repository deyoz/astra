CREATE TABLE WB_REF_WS_TYPES (
DATE_WRITE DATE NOT NULL,
DOP_IDENT VARCHAR2(50) NOT NULL,
IATA VARCHAR2(50) NOT NULL,
ICAO VARCHAR2(50) NOT NULL,
ID NUMBER NOT NULL,
NAME_ENG_FULL VARCHAR2(200) NOT NULL,
NAME_ENG_SMALL VARCHAR2(100) NOT NULL,
NAME_RUS_FULL VARCHAR2(200) NOT NULL,
NAME_RUS_SMALL VARCHAR2(100) NOT NULL,
REMARK CLOB NOT NULL,
U_HOST_NAME VARCHAR2(100) NOT NULL,
U_IP VARCHAR2(100) NOT NULL,
U_NAME VARCHAR2(100) NOT NULL
);