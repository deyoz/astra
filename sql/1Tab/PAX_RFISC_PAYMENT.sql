CREATE TABLE PAX_RFISC_PAYMENT (
AIRLINE VARCHAR2(3) NOT NULL,
DOC_COUPON NUMBER(1),
DOC_NO VARCHAR2(15) NOT NULL,
DOC_TYPE VARCHAR2(5) NOT NULL,
LIST_ID NUMBER(9) NOT NULL,
PAX_ID NUMBER(9) NOT NULL,
RFISC VARCHAR2(15) NOT NULL,
SERVICE_QUANTITY NUMBER(3) NOT NULL,
SERVICE_TYPE VARCHAR2(1) NOT NULL,
TRANSFER_NUM NUMBER(1) NOT NULL
);