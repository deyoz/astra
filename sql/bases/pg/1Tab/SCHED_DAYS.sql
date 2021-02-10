CREATE TABLE SCHED_DAYS (
    DAYS VARCHAR(7) NOT NULL,
    DELTA SMALLINT,
    FIRST_DAY TIMESTAMP(0) NOT NULL,
    LAST_DAY TIMESTAMP(0) NOT NULL,
    MOVE_ID INTEGER NOT NULL,
    NUM SMALLINT NOT NULL,
    PR_DEL SMALLINT DEFAULT 0 NOT NULL,
    REFERENCE VARCHAR(255),
    REGION VARCHAR(50) NOT NULL,
    SSM_ID INTEGER,
    TLG VARCHAR(10),
    TRIP_ID INTEGER NOT NULL
);
