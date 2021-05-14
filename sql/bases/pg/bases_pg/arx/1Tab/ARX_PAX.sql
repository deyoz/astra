CREATE TABLE ARX_PAX (
    BAG_POOL_NUM SMALLINT,
    CABIN_CLASS VARCHAR(1),
    CABIN_CLASS_GRP INTEGER,
    CABIN_SUBCLASS VARCHAR(1),
    COUPON_NO SMALLINT,
    DOCO_CONFIRM SMALLINT,
    EXCESS_PC SMALLINT,
    GRP_ID INTEGER NOT NULL,
    IS_FEMALE SMALLINT,
    IS_JMP SMALLINT,
    NAME VARCHAR(64),
    PART_KEY TIMESTAMP(0) NOT NULL,
    PAX_ID INTEGER NOT NULL,
    PERS_TYPE VARCHAR(2) NOT NULL,
    PR_BRD SMALLINT DEFAULT 0,
    PR_EXAM SMALLINT NOT NULL,
    REFUSE VARCHAR(1),
    REG_NO SMALLINT NOT NULL,
    SEATS SMALLINT DEFAULT 1 NOT NULL,
    SEAT_NO VARCHAR(8),
    SEAT_TYPE VARCHAR(4),
    SUBCLASS VARCHAR(1),
    SURNAME VARCHAR(64) NOT NULL,
    TICKET_CONFIRM SMALLINT NOT NULL,
    TICKET_NO VARCHAR(15),
    TICKET_REM VARCHAR(5),
    TID INTEGER NOT NULL,
    WL_TYPE VARCHAR(1)
);
