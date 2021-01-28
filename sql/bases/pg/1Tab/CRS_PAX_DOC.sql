CREATE TABLE CRS_PAX_DOC (
    BIRTH_DATE TIMESTAMP(0),
    EXPIRY_DATE TIMESTAMP(0),
    FIRST_NAME VARCHAR(64),
    GENDER VARCHAR(2),
    ISSUE_COUNTRY VARCHAR(3),
    NATIONALITY VARCHAR(3),
    NO VARCHAR(15),
    PAX_ID INTEGER NOT NULL,
    PR_MULTI SMALLINT NOT NULL,
    REM_CODE VARCHAR(5) NOT NULL,
    REM_STATUS VARCHAR(2),
    SECOND_NAME VARCHAR(64),
    SURNAME VARCHAR(64),
    TYPE VARCHAR(2),
    TYPE_RCPT VARCHAR(3),
    UPDATE_DATETIME TIMESTAMP(0)
);

