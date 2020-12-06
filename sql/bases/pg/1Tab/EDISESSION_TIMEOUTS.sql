CREATE TABLE EDISESSION_TIMEOUTS (
    FUNC_CODE VARCHAR(3),
    INSTANCE VARCHAR(3),
    MSG_NAME VARCHAR(6) NOT NULL,
    SESS_IDA BIGINT NOT NULL,
    TIME_OUT TIMESTAMP(0) NOT NULL,
    CONSTRAINT EDISESSION_TIMEOUTS_IDA_PK PRIMARY KEY (SESS_IDA)
);
