CREATE TABLE BAG_PAY_TYPES (
    EXTRA VARCHAR(50),
    NUM SMALLINT NOT NULL,
    PAY_RATE_SUM DECIMAL(12,2) NOT NULL,
    PAY_TYPE VARCHAR(4) NOT NULL,
    RECEIPT_ID INTEGER NOT NULL
);

