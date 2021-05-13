CREATE TABLE ARX_PAY_SERVICES (
    AIRLINE VARCHAR2(3) NOT NULL,
    CURRENCY VARCHAR2(3) NOT NULL,
    DOC_ID VARCHAR2(100) NOT NULL,
    GRP_ID NUMBER(9) NOT NULL,
    LIST_ID NUMBER(9),
    NAME_VIEW VARCHAR2(100) NOT NULL,
    NAME_VIEW_LAT VARCHAR2(100) NOT NULL,
    ORDER_ID VARCHAR2(6) NOT NULL,
    PART_KEY DATE NOT NULL,
    PASS_ID NUMBER(9) NOT NULL,
    PAX_ID NUMBER(9) NOT NULL,
    PRICE FLOAT NOT NULL,
    RFISC VARCHAR2(15) NOT NULL,
    SEG_ID NUMBER(9) NOT NULL,
    SERVICE_TYPE VARCHAR2(1) NOT NULL,
    SVC_ID NUMBER(9) NOT NULL,
    TICKET_CPN VARCHAR2(1) NOT NULL,
    TICKNUM VARCHAR2(15) NOT NULL,
    TIME_PAID DATE NOT NULL,
    TRANSFER_NUM NUMBER(1) NOT NULL
);