CREATE TABLE BAG_RATES (
    AIRLINE VARCHAR2(3),
    BAG_TYPE NUMBER(2),
    CITY_ARV VARCHAR2(3),
    CITY_DEP VARCHAR2(3),
    CLASS VARCHAR2(1),
    CRAFT VARCHAR2(3),
    EXTRA VARCHAR2(2000),
    FIRST_DATE DATE NOT NULL,
    FLT_NO NUMBER(5),
    ID NUMBER(9) NOT NULL,
    LAST_DATE DATE,
    MIN_WEIGHT NUMBER(4),
    PAX_CAT VARCHAR2(4),
    PR_DEL NUMBER(1) NOT NULL,
    PR_TRFER NUMBER(1),
    RATE NUMBER(8,2) NOT NULL,
    RATE_CUR VARCHAR2(3) NOT NULL,
    SUBCLASS VARCHAR2(1),
    TID NUMBER(9) NOT NULL,
    TRIP_TYPE VARCHAR2(1)
);
