CREATE TABLE HIST_BAG_NORMS (
AIRLINE VARCHAR2(3),
AMOUNT NUMBER(2),
BAG_TYPE NUMBER(2),
CITY_ARV VARCHAR2(3),
CITY_DEP VARCHAR2(3),
CLASS VARCHAR2(1),
CRAFT VARCHAR2(3),
EXTRA VARCHAR2(2000),
FIRST_DATE DATE NOT NULL,
FLT_NO NUMBER(5),
HIST_ORDER NUMBER(9) NOT NULL,
HIST_TIME DATE NOT NULL,
ID NUMBER(9) NOT NULL,
LAST_DATE DATE,
NORM_TYPE VARCHAR2(5) NOT NULL,
PAX_CAT VARCHAR2(4),
PER_UNIT NUMBER(1),
PR_DEL NUMBER(1) NOT NULL,
PR_TRFER NUMBER(1),
SUBCLASS VARCHAR2(1),
TRIP_TYPE VARCHAR2(1),
WEIGHT NUMBER(3)
);
