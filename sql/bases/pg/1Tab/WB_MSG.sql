CREATE TABLE WB_MSG (
    ID INTEGER NOT NULL,
    MSG_TYPE VARCHAR(20) NOT NULL,
    POINT_ID INTEGER NOT NULL,
    SOURCE VARCHAR(20),
    TIME_RECEIVE TIMESTAMP(0) NOT NULL
);

