create or replace TYPE        "TYPEB_ADDR_OPTIONS_TYPE"                                          as object
(
TLG_TYPE       VARCHAR2(6),
AIRLINE        VARCHAR2(3),
FLT_NO         NUMBER(5),
AIRP_DEP       VARCHAR2(3),
AIRP_ARV       VARCHAR2(3),
CRS            VARCHAR2(7),
ADDR           VARCHAR2(100),
PR_LAT         NUMBER(1),
PR_MARK_FLT    NUMBER(1),
PR_MARK_HEADER NUMBER(1)
);
/
