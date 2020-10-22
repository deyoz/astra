CREATE TABLE CACHE_TABLES (
CODE VARCHAR2(30) NOT NULL,
DELETE_RIGHT NUMBER(4),
DELETE_SQL VARCHAR2(4000),
EVENT_TYPE VARCHAR2(3),
INSERT_RIGHT NUMBER(4),
INSERT_SQL VARCHAR2(4000),
KEEP_DELETED_ROWS NUMBER(1) DEFAULT 0 NOT NULL,
KEEP_LOCALLY NUMBER(1) DEFAULT 0 NOT NULL,
LOGGING NUMBER(1) NOT NULL,
NEED_REFRESH NUMBER(1) DEFAULT 1 NOT NULL,
REFRESH_SQL VARCHAR2(4000),
SELECT_RIGHT NUMBER(4),
SELECT_SQL VARCHAR2(4000) NOT NULL,
TID NUMBER(9) NOT NULL,
TITLE VARCHAR2(100) NOT NULL,
UPDATE_RIGHT NUMBER(4),
UPDATE_SQL VARCHAR2(4000)
);