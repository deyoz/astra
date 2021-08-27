CREATE TABLE TIMATIC_SESSION (
    EXPIRE TIMESTAMP(0) NOT NULL,
    HOST VARCHAR(200) NOT NULL,
    JSESSIONID VARCHAR(200),
    PORT INTEGER NOT NULL,
    PWD VARCHAR(200) NOT NULL,
    SESSIONID VARCHAR(200),
    SUB_USERNAME VARCHAR(200) NOT NULL,
    USERNAME VARCHAR(200) NOT NULL
);
