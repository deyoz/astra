CREATE TABLE EMDOCS (
ASSOCIATED NUMBER(1) NOT NULL,
ASSOCIATED_COUPON NUMBER(1),
ASSOCIATED_NO VARCHAR2(15),
CHANGE_STATUS_ERROR VARCHAR2(100),
COUPON_NO NUMBER(1) NOT NULL,
COUPON_STATUS VARCHAR2(1),
DOC_NO VARCHAR2(15) NOT NULL,
POINT_ID NUMBER(9) NOT NULL,
SYSTEM_UPDATE_ERROR VARCHAR2(100)
);
