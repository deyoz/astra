CREATE TABLE WB_REF_AIRCO_AUTO_DOC_ADV_HIST (
ACTION_ VARCHAR2(100) NOT NULL,
DATE_FROM_NEW DATE,
DATE_FROM_OLD DATE,
DATE_WRITE_ DATE NOT NULL,
DATE_WRITE_NEW DATE NOT NULL,
DATE_WRITE_OLD DATE NOT NULL,
ID NUMBER NOT NULL,
ID_ NUMBER NOT NULL,
ID_AC_NEW NUMBER NOT NULL,
ID_AC_OLD NUMBER NOT NULL,
REMARK_NEW CLOB,
REMARK_OLD CLOB,
U_HOST_NAME_ VARCHAR2(100) NOT NULL,
U_HOST_NAME_NEW VARCHAR2(100) NOT NULL,
U_HOST_NAME_OLD VARCHAR2(100) NOT NULL,
U_IP_ VARCHAR2(100) NOT NULL,
U_IP_NEW VARCHAR2(100) NOT NULL,
U_IP_OLD VARCHAR2(100) NOT NULL,
U_NAME_ VARCHAR2(100) NOT NULL,
U_NAME_NEW VARCHAR2(100) NOT NULL,
U_NAME_OLD VARCHAR2(100) NOT NULL
);