CREATE TABLE DESKS (
    CODE VARCHAR(6) NOT NULL,
    CURRENCY VARCHAR(3) NOT NULL,
    GRP_ID INTEGER NOT NULL,
    ID INTEGER NOT NULL,
    LAST_LOGON TIMESTAMP(0),
    TERM_ID BIGINT,
    TERM_MODE VARCHAR(10),
    UNDER_CONSTR SMALLINT,
    VERSION VARCHAR(20)
);
