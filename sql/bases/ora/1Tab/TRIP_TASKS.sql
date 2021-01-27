CREATE TABLE TRIP_TASKS (
    ID NUMBER(9) NOT NULL,
    LAST_EXEC DATE,
    NAME VARCHAR2(50) NOT NULL,
    NEXT_EXEC DATE,
    PARAMS VARCHAR2(4000),
    POINT_ID NUMBER(9) NOT NULL,
    PROC_NAME VARCHAR2(20),
    TID NUMBER(9)
);
