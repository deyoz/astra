CREATE TABLE WB_REF_AIRCO_PORT_FLIGHT_HIST (
ACTION_ VARCHAR2(100) NOT NULL,
DATE_FROM_NEW DATE NOT NULL,
DATE_FROM_OLD DATE NOT NULL,
DATE_WRITE_ DATE NOT NULL,
DATE_WRITE_NEW DATE NOT NULL,
DATE_WRITE_OLD DATE NOT NULL,
ID NUMBER NOT NULL,
ID_ NUMBER NOT NULL,
ID_AC_NEW NUMBER NOT NULL,
ID_AC_OLD NUMBER NOT NULL,
ID_PORT_NEW NUMBER NOT NULL,
ID_PORT_OLD NUMBER NOT NULL,
IS_CHECK_IN_NEW NUMBER NOT NULL,
IS_CHECK_IN_OLD NUMBER NOT NULL,
IS_LOAD_CONTROL_NEW NUMBER NOT NULL,
IS_LOAD_CONTROL_OLD NUMBER NOT NULL,
UTC_DIFF_NEW NUMBER NOT NULL,
UTC_DIFF_OLD NUMBER NOT NULL,
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