CREATE TABLE POINTS_LOCK_EVENTS (
DESK VARCHAR2(30) NOT NULL,
LOCK_MSEC NUMBER(15) NOT NULL,
LOCK_ORDER NUMBER(9) NOT NULL,
LOCK_TIME DATE NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
WAIT_MSEC NUMBER(15) NOT NULL,
WHENCE VARCHAR2(100) NOT NULL
);
