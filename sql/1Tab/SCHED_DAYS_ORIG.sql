CREATE TABLE SCHED_DAYS_ORIG (
DAYS VARCHAR2(7),
FIRST_DAY DATE,
LAST_DAY DATE,
MOVE_ID NUMBER(9),
NUM NUMBER(3),
PR_DEL NUMBER(1) DEFAULT 0,
REFERENCE VARCHAR2(255),
REGION VARCHAR2(50),
TLG VARCHAR2(10),
TRIP_ID NUMBER(9)
);
