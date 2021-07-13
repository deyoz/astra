CREATE TABLE APIS_SETS (
    AIRLINE VARCHAR(3) NOT NULL,
    COUNTRY_ARV VARCHAR(2),
    COUNTRY_CONTROL VARCHAR(2) NOT NULL,
    COUNTRY_DEP VARCHAR(2),
    EDI_ADDR VARCHAR(40),
    EDI_OWN_ADDR VARCHAR(40),
    FORMAT VARCHAR(10) NOT NULL,
    ID INTEGER NOT NULL,
    PR_DENIAL SMALLINT NOT NULL,
    TRANSPORT_PARAMS VARCHAR(250) NOT NULL,
    TRANSPORT_TYPE VARCHAR(20) NOT NULL
);

