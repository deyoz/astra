CREATE TABLE EMDOCS (
    ASSOCIATED SMALLINT NOT NULL,
    ASSOCIATED_COUPON SMALLINT,
    ASSOCIATED_NO VARCHAR(15),
    CHANGE_STATUS_ERROR VARCHAR(100),
    COUPON_NO SMALLINT NOT NULL,
    COUPON_STATUS VARCHAR(1),
    DOC_NO VARCHAR(15) NOT NULL,
    POINT_ID INTEGER NOT NULL,
    SYSTEM_UPDATE_ERROR VARCHAR(100)
);
