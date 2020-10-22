CREATE TABLE HIST_CKIN_CLIENT_SETS (
AIRLINE VARCHAR2(3),
AIRP_DEP VARCHAR2(3),
CLIENT_TYPE VARCHAR2(5) NOT NULL,
DESK_GRP_ID NUMBER(9),
FLT_NO NUMBER(5),
HIST_ORDER NUMBER(9) NOT NULL,
HIST_TIME DATE NOT NULL,
ID NUMBER(9) NOT NULL,
PR_PERMIT NUMBER(1) NOT NULL,
PR_TCKIN NUMBER(1) NOT NULL,
PR_UPD_STAGE NUMBER(1) NOT NULL,
PR_WAITLIST NUMBER(1) NOT NULL
);