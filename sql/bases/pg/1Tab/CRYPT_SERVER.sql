CREATE TABLE CRYPT_SERVER (
    CERTIFICATE VARCHAR(4000) NOT NULL,
    FIRST_DATE TIMESTAMP(0) NOT NULL,
    ID INTEGER NOT NULL,
    LAST_DATE TIMESTAMP(0) NOT NULL,
    PRIVATE_KEY VARCHAR(4000),
    PR_CA SMALLINT NOT NULL,
    PR_DENIAL SMALLINT NOT NULL
);
