CREATE TABLE DESK_NOTICES (
    ALWAYS_ENABLED SMALLINT NOT NULL,
    DEFAULT_DISABLE SMALLINT NOT NULL,
    DESK VARCHAR(6),
    DESK_GRP_ID INTEGER,
    FIRST_VERSION VARCHAR(20),
    LAST_VERSION VARCHAR(20),
    NOTICE_ID INTEGER NOT NULL,
    NOTICE_TYPE SMALLINT,
    PR_DEL SMALLINT NOT NULL,
    TERM_MODE VARCHAR(10),
    TIME_CREATE TIMESTAMP(0) NOT NULL
);

