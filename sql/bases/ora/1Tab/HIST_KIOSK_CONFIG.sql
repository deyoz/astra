CREATE TABLE HIST_KIOSK_CONFIG (
    APP_ID NUMBER(9),
    DESCR VARCHAR2(1000),
    GRP_ID NUMBER(9),
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    ID NUMBER(9) NOT NULL,
    KIOSK_ID VARCHAR2(100),
    NAME VARCHAR2(500) NOT NULL,
    PR_DEL NUMBER(1) NOT NULL,
    VALUE VARCHAR2(2000)
);
