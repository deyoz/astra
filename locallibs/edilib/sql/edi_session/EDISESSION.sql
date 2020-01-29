CREATE TABLE EDISESSION (
APTIDA NUMBER(5),
IDA NUMBER(9) NOT NULL,
INTMSGID VARCHAR2(24) NOT NULL,
LAST_ACCESS DATE NOT NULL,
OTHERCARF VARCHAR2(50),
OTHERREF VARCHAR2(14),
OTHERREFNUM NUMBER(4),
OURREF VARCHAR2(14) NOT NULL,
OURREFNUM NUMBER(4),
PULT VARCHAR2(6),
SESSDATECR DATE NOT NULL,
STATUS VARCHAR2(1) NOT NULL,
PRED_P VARCHAR2(6),
MSG_ID NUMBER,
OURCARF VARCHAR2(17) NOT NULL,
FLAGS NUMBER,
TIMEOUT DATE,

SYSTEM_ID NUMBER,
SYSTEM_TYPE  VARCHAR2(4),
SESSION_TYPE VARCHAR(3),
ACCESS_KEY VARCHAR(10)
);