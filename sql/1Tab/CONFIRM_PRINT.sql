CREATE TABLE CONFIRM_PRINT (
CLIENT_TYPE VARCHAR2(5) NOT NULL,
DESK VARCHAR2(6) NOT NULL,
HALL_ID NUMBER(9),
OP_TYPE VARCHAR2(10),
PAX_ID NUMBER(9) NOT NULL,
PR_PRINT NUMBER(1) NOT NULL,
SEAT_NO VARCHAR2(50),
SEAT_NO_LAT VARCHAR2(50),
TIME_PRINT DATE NOT NULL,
VOUCHER VARCHAR2(2),
FROM_SCAN_CODE NUMBER(1)
);
