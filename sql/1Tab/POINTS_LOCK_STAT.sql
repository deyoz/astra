CREATE TABLE POINTS_LOCK_STAT (
LOCK_COUNT NUMBER(9) NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
WAIT_TOTAL NUMBER(15) NOT NULL,
WHENCE VARCHAR2(100) NOT NULL
);
