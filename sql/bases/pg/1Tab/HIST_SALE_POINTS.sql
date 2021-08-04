CREATE TABLE HIST_SALE_POINTS (
    AGENCY VARCHAR(5) NOT NULL,
    CITY VARCHAR(3) NOT NULL,
    CODE VARCHAR(8) NOT NULL,
    DESCR VARCHAR(30) NOT NULL,
    DESCR_LAT VARCHAR(30),
    FORMS VARCHAR(30),
    HIST_ORDER INTEGER NOT NULL,
    HIST_TIME TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    PHONE VARCHAR(20),
    PR_PERMIT SMALLINT NOT NULL,
    VALIDATOR VARCHAR(4) NOT NULL
);

