CREATE TABLE IAPI_PAX_DATA (
    COUNTRY_CONTROL VARCHAR(2) NOT NULL,
    FREE_TEXT VARCHAR(100),
    MSG_ID VARCHAR(70) NOT NULL,
    PAX_ID BIGINT NOT NULL,
    POINT_ID INTEGER NOT NULL,
    PR_DEL SMALLINT NOT NULL,
    STATUS VARCHAR(2)
);