create table IATCI_SEG
(
AIRLINE VARCHAR2(3) NOT NULL,
FLT_NO NUMBER(5) NOT NULL,
DEP_PORT VARCHAR2(3) NOT NULL,
ARR_PORT VARCHAR2(3) NOT NULL,
DEP_DATE DATE,
DEP_TIME DATE,
ARR_DATE DATE,
ARR_TIME DATE,
NUM NUMBER(2) NOT NULL,
GRP_ID NUMBER(9)
);
