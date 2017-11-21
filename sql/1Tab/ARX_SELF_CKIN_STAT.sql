CREATE TABLE ARX_SELF_CKIN_STAT (
ADULT NUMBER(3) NOT NULL,
BABY NUMBER(3) NOT NULL,
CHILD NUMBER(3) NOT NULL,
CLIENT_TYPE VARCHAR2(5) NOT NULL,
DESCR VARCHAR2(100) NOT NULL,
DESK VARCHAR2(6) NOT NULL,
DESK_AIRP VARCHAR2(3),
PART_KEY DATE NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
TCKIN NUMBER(3) NOT NULL,
TERM_BAG NUMBER(3),
TERM_BP NUMBER(3),
TERM_CKIN_SERVICE NUMBER(3)
);