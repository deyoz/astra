CREATE TABLE EDISESSION_TIMEOUTS (
    FUNC_CODE VARCHAR2(3),
    INSTANCE VARCHAR2(3),
    MSG_NAME VARCHAR2(6) NOT NULL,
    SESS_IDA NUMBER NOT NULL,
    TIME_OUT DATE NOT NULL
);
