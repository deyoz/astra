CREATE TABLE ARX_POINTS (
ACT_IN DATE,
ACT_OUT DATE,
AIRLINE VARCHAR2(3),
AIRLINE_FMT NUMBER(1),
AIRP VARCHAR2(3) NOT NULL,
AIRP_FMT NUMBER(1) NOT NULL,
BORT VARCHAR2(10),
CRAFT VARCHAR2(3),
CRAFT_FMT NUMBER(1),
EST_IN DATE,
EST_OUT DATE,
FIRST_POINT NUMBER(9),
FLT_NO NUMBER(5),
LITERA VARCHAR2(3),
MOVE_ID NUMBER(9) NOT NULL,
PARK_IN VARCHAR2(3),
PARK_OUT VARCHAR2(3),
PART_KEY DATE NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
POINT_NUM NUMBER(5) NOT NULL,
PR_DEL NUMBER(1) DEFAULT 0 NOT NULL,
PR_REG NUMBER(1) NOT NULL,
PR_TRANZIT NUMBER(1) NOT NULL,
REMARK VARCHAR2(250),
SCD_IN DATE,
SCD_OUT DATE,
SUFFIX VARCHAR2(1),
SUFFIX_FMT NUMBER(1),
TID NUMBER(9) NOT NULL,
TRIP_TYPE VARCHAR2(1)
);
