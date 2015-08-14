CREATE TABLE PAX_GRP (
AIRP_ARV VARCHAR2(3) NOT NULL,
AIRP_DEP VARCHAR2(3) NOT NULL,
BAG_REFUSE NUMBER(1) NOT NULL,
CLASS VARCHAR2(1),
CLASS_GRP NUMBER(9),
CLIENT_TYPE VARCHAR2(5) NOT NULL,
EXCESS NUMBER(4) NOT NULL,
GRP_ID NUMBER(9) NOT NULL,
HALL NUMBER(9),
INBOUND_CONFIRM NUMBER(1) NOT NULL,
POINT_ARV NUMBER(9) NOT NULL,
POINT_DEP NUMBER(9) NOT NULL,
POINT_ID_MARK NUMBER(9) NOT NULL,
PR_MARK_NORMS NUMBER(1) NOT NULL,
STATUS VARCHAR2(1) NOT NULL,
TID NUMBER(9) NOT NULL,
TRFER_CONFIRM NUMBER(1) NOT NULL,
TRFER_CONFLICT NUMBER(1) NOT NULL,
USER_ID NUMBER(9)
);
