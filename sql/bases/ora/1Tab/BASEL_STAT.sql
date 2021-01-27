CREATE TABLE BASEL_STAT (
    AIRP VARCHAR2(3) NOT NULL,
    PAX_ID NUMBER(9),
    POINT_ID NUMBER(9) NOT NULL,
    TIME_CREATE DATE NOT NULL,
    VIEWBAGNORMS VARCHAR2(200),
    VIEWBOARDINGTIME DATE,
    VIEWCARRYON NUMBER(9),
    VIEWCHECKINDURATION DATE,
    VIEWCHECKINNO NUMBER(3),
    VIEWCHECKINTIME DATE,
    VIEWCLASS VARCHAR2(10),
    VIEWCLIENTTYPE VARCHAR2(5),
    VIEWDATE DATE,
    VIEWDEPARTUREPLANTIME DATE,
    VIEWDEPARTUREREALTIME DATE,
    VIEWFLIGHT VARCHAR2(9),
    VIEWGROUP NUMBER(9),
    VIEWNAME VARCHAR2(130),
    VIEWPAYWEIGHT NUMBER(9),
    VIEWPCT NUMBER(9),
    VIEWPCTWEIGHTPAIDBYTYPE VARCHAR2(200),
    VIEWSTATION VARCHAR2(6),
    VIEWSTATUS VARCHAR2(30),
    VIEWTAG VARCHAR2(100),
    VIEWUNCHECKIN VARCHAR2(50),
    VIEWWEIGHT NUMBER(9)
);
