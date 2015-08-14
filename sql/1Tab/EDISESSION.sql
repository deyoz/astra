CREATE TABLE EDISESSION (
ACCESS_KEY VARCHAR2(10),
FLAGS NUMBER,
IDA NUMBER(9) NOT NULL,
LAST_ACCESS DATE NOT NULL,
MSG_ID NUMBER,
OTHERCARF VARCHAR2(50),
OTHERREF VARCHAR2(14),
OTHERREFNUM NUMBER(4),
OURCARF VARCHAR2(17) NOT NULL,
OURREF VARCHAR2(14) NOT NULL,
OURREFNUM NUMBER(4) NOT NULL,
PRED_P VARCHAR2(6),
PULT VARCHAR2(6),
SESSDATECR DATE NOT NULL,
SESSION_TYPE VARCHAR2(4),
STATUS VARCHAR2(1) NOT NULL,
SYSTEM_ID NUMBER(9),
SYSTEM_TYPE VARCHAR2(3),
TIMEOUT DATE
);
