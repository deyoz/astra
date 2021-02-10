CREATE TABLE HIST_KIOSK_CLIENTS (
    ADDR_ID NUMBER(9),
    APP_ID NUMBER(9) NOT NULL,
    CLIENT_ID VARCHAR2(20) NOT NULL,
    DESCR VARCHAR2(100) NOT NULL,
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    ID NUMBER(9) NOT NULL,
    KIOSK_ID VARCHAR2(50) NOT NULL
);