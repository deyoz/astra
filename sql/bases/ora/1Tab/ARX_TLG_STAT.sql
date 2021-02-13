CREATE TABLE ARX_TLG_STAT (
    AIRLINE VARCHAR2(3),
    AIRLINE_MARK VARCHAR2(3),
    AIRP_DEP VARCHAR2(3),
    EXTRA VARCHAR2(50) NOT NULL,
    FLT_NO NUMBER(5),
    PART_KEY DATE NOT NULL,
    QUEUE_TLG_ID NUMBER(9) NOT NULL,
    RECEIVER_CANON_NAME VARCHAR2(6) NOT NULL,
    RECEIVER_COUNTRY VARCHAR2(2),
    RECEIVER_DESCR VARCHAR2(50) NOT NULL,
    RECEIVER_SITA_ADDR VARCHAR2(7) NOT NULL,
    SCD_LOCAL_DATE DATE,
    SENDER_CANON_NAME VARCHAR2(6) NOT NULL,
    SENDER_COUNTRY VARCHAR2(2),
    SENDER_DESCR VARCHAR2(50) NOT NULL,
    SENDER_SITA_ADDR VARCHAR2(7) NOT NULL,
    SUFFIX VARCHAR2(1),
    TIME_CREATE DATE NOT NULL,
    TIME_RECEIVE DATE,
    TIME_SEND DATE,
    TLG_LEN NUMBER(9) NOT NULL,
    TLG_TYPE VARCHAR2(10) NOT NULL,
    TYPEB_TLG_ID NUMBER(9) NOT NULL,
    TYPEB_TLG_NUM NUMBER(5) NOT NULL
);
