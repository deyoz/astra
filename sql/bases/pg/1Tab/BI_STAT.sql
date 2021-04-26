CREATE TABLE BI_STAT (
    DESK VARCHAR(6) NOT NULL,
    HALL INTEGER NOT NULL,
    OP_TYPE VARCHAR(10) NOT NULL,
    PAX_ID INTEGER NOT NULL,
    POINT_ID INTEGER NOT NULL,
    PRINT_TYPE VARCHAR(3) NOT NULL,
    PR_PRINT SMALLINT NOT NULL,
    SCD_OUT TIMESTAMP(0) NOT NULL,
    TERMINAL INTEGER,
    TIME_PRINT TIMESTAMP(0) NOT NULL
);

