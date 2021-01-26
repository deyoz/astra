CREATE TABLE HIST_EXTRA_USER_ACCESS (
    AIRLINE_FROM VARCHAR2(3),
    AIRLINE_TO VARCHAR2(3),
    AIRP_FROM VARCHAR2(3),
    AIRP_TO VARCHAR2(3),
    FULL_ACCESS NUMBER(1) NOT NULL,
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    ID NUMBER(9) NOT NULL,
    TYPE_FROM NUMBER(1) NOT NULL,
    TYPE_TO NUMBER(1) NOT NULL
);
