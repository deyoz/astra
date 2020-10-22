create or replace TYPE        "SALE_DESKS_TYPE"                                          as object
(
CODE       VARCHAR2(6),
SALE_POINT VARCHAR2(8),
VALIDATOR  VARCHAR2(4),
PR_DENIAL  NUMBER(1)
);
/
