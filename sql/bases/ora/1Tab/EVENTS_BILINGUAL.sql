CREATE TABLE EVENTS_BILINGUAL (
    EV_ORDER NUMBER(9) NOT NULL,
    EV_USER VARCHAR2(20),
    ID1 NUMBER(9),
    ID2 NUMBER(9),
    ID3 NUMBER(9),
    LANG VARCHAR2(2) NOT NULL,
    MSG VARCHAR2(250) NOT NULL,
    PART_NUM NUMBER(2) NOT NULL,
    SCREEN VARCHAR2(15),
    STATION VARCHAR2(15),
    SUB_TYPE VARCHAR2(100),
    TIME DATE NOT NULL,
    TYPE VARCHAR2(3) NOT NULL
/*
) PARTITION BY LIST (LANG) (
    PARTITION EVENTS_EN VALUES('EN'),
    PARTITION EVENTS_RU VALUES('RU')
*/
);
