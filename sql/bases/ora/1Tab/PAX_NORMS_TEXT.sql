CREATE TABLE PAX_NORMS_TEXT (
AIRLINE VARCHAR2(3) NOT NULL,
CARRY_ON NUMBER(1) NOT NULL,
CONCEPT VARCHAR2(10) NOT NULL,
LANG VARCHAR2(2) NOT NULL,
PAGE_NO NUMBER(9) NOT NULL,
PAX_ID NUMBER(9) NOT NULL,
RFISCS VARCHAR2(50),
TEXT VARCHAR2(4000) NOT NULL,
TRANSFER_NUM NUMBER(1) NOT NULL
);
