CREATE TABLE BOOST_SCHED_DAYS (
DAYS VARCHAR2(7) NOT NULL,
FIRST_DAY DATE NOT NULL,
LAST_DAY DATE NOT NULL,
MOVE_ID NUMBER(9) NOT NULL,
NUM NUMBER(3) NOT NULL,
PR_DEL NUMBER(1) DEFAULT 0 NOT NULL,
REFERENCE VARCHAR2(255),
REGION VARCHAR2(50) NOT NULL,
TLG VARCHAR2(10),
TRIP_ID NUMBER(9) NOT NULL
);
