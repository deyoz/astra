CREATE TABLE WB_REF_TMP_AIRCOMPANY_AHM_TREE (
DATA_BLOCK VARCHAR2(100),
FACT_DB_ID NUMBER DEFAULT 0 ,
FUNCTIONS VARCHAR2(500) DEFAULT 'EMPTY_STRING' ,
ID NUMBER NOT NULL,
LEVEL_TREE NUMBER,
PARENT_ID NUMBER NOT NULL,
SHOW_INFO NUMBER DEFAULT 0 ,
SORT_PRIOR_ENG NUMBER NOT NULL,
SORT_PRIOR_RUS NUMBER NOT NULL,
TAB_INDEX NUMBER,
TITLE_ENG VARCHAR2(100) NOT NULL,
TITLE_RUS VARCHAR2(100) NOT NULL,
WS_DB_ID NUMBER DEFAULT 0
);