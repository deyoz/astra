CREATE TABLE HIST_SELF_CKIN_SET (
    AIRLINE VARCHAR2(3),
    AIRP_DEP VARCHAR2(3),
    CLIENT_TYPE VARCHAR2(5),
    FLT_NO NUMBER(5),
    HIST_ORDER NUMBER(9) NOT NULL,
    HIST_TIME DATE NOT NULL,
    ID NUMBER(9) NOT NULL,
    TYPE NUMBER(3) NOT NULL,
    VALUE NUMBER(1) NOT NULL
);
