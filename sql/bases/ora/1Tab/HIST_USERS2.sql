CREATE TABLE HIST_USERS2 (
    DESCR VARCHAR2(20) NOT NULL,
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    LOGIN VARCHAR2(20),
    PASSWD VARCHAR2(20),
    PR_DENIAL NUMBER(1) NOT NULL,
    TYPE NUMBER(1) NOT NULL,
    USER_ID NUMBER(9) NOT NULL
);
