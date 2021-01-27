CREATE TABLE BAG_PREPAY (
    AIRCODE VARCHAR2(3) NOT NULL,
    BAG_TYPE NUMBER(2),
    EX_WEIGHT NUMBER(4),
    GRP_ID NUMBER(9) NOT NULL,
    NO VARCHAR2(15) NOT NULL,
    RECEIPT_ID NUMBER(9) NOT NULL,
    VALUE NUMBER(12,2),
    VALUE_CUR VARCHAR2(3)
);
