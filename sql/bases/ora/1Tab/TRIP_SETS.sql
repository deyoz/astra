CREATE TABLE TRIP_SETS (
APIS_CONTROL NUMBER(1) NOT NULL,
APIS_MANUAL_INPUT NUMBER(1) NOT NULL,
AUTO_COMP_CHG NUMBER(1) DEFAULT 1 NOT NULL,
AUTO_WEIGHING NUMBER(1) NOT NULL,
C NUMBER(3),
COMP_ID NUMBER(9),
CRC_COMP NUMBER(10) DEFAULT 0 NOT NULL,
CRC_BASE_COMP NUMBER(10) DEFAULT 0,
ET_FINAL_ATTEMPT NUMBER(1) DEFAULT 0 NOT NULL,
F NUMBER(3),
JMP_CFG NUMBER(3) NOT NULL,
LCI_PERS_WEIGHTS NUMBER(1),
MAX_COMMERCE NUMBER(6) DEFAULT 0,
PIECE_CONCEPT NUMBER(1) NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
PR_BASEL_STAT NUMBER(1) NOT NULL,
PR_BLOCK_TRZT NUMBER(1) DEFAULT 0,
PR_CHECK_LOAD NUMBER(1) NOT NULL,
PR_CHECK_PAY NUMBER(1) NOT NULL,
PR_ETSTATUS NUMBER(1) NOT NULL,
PR_EXAM NUMBER(1) NOT NULL,
PR_EXAM_CHECK_PAY NUMBER(1) NOT NULL,
PR_FREE_SEATING NUMBER(1) NOT NULL,
PR_LAT_SEAT NUMBER(1),
PR_OVERLOAD_REG NUMBER(1) NOT NULL,
PR_REG_WITHOUT_TKNA NUMBER(1) NOT NULL,
PR_REG_WITH_DOC NUMBER(1) NOT NULL,
PR_REG_WITH_TKN NUMBER(1) NOT NULL,
PR_STAT NUMBER(1) NOT NULL,
PR_TRANZ_REG NUMBER(1),
TRZT_BORT_CHANGING NUMBER(1),
TRZT_BRD_WITH_AUTOREG NUMBER(1),
USE_JMP NUMBER(1) NOT NULL,
Y NUMBER(3)
);
