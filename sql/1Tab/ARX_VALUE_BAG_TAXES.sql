CREATE TABLE ARX_VALUE_BAG_TAXES (
AIRLINE VARCHAR2(3),
CITY_ARV VARCHAR2(3),
CITY_DEP VARCHAR2(3),
EXTRA VARCHAR2(2000),
FIRST_DATE DATE NOT NULL,
ID NUMBER(9) NOT NULL,
LAST_DATE DATE,
MIN_VALUE NUMBER(8),
MIN_VALUE_CUR VARCHAR2(3),
PART_KEY DATE NOT NULL,
PR_DEL NUMBER(1) NOT NULL,
PR_TRFER NUMBER(1),
TAX NUMBER(4) NOT NULL,
TID NUMBER(9) NOT NULL
);
