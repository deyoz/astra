CREATE TABLE HIST_WEB_CLIENTS (
    CLIENT_ID VARCHAR2(20),
    CLIENT_TYPE VARCHAR2(5) NOT NULL,
    DESCR VARCHAR2(100) NOT NULL,
    DESK VARCHAR2(6) NOT NULL,
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    ID NUMBER(9) NOT NULL,
    TRACING_SEARCH NUMBER(1) NOT NULL,
    USER_ID NUMBER(9) NOT NULL,
    KIOSK_ADDR NUMBER(9),
    KIOSK_ID VARCHAR2(5)
);
