CREATE TABLE APPS_PAX_DATA (
APPS_PAX_ID VARCHAR(15),
CICX_MSG_ID INTEGER,
CIRQ_MSG_ID INTEGER NOT NULL,
CIRQ_MSG_TEXT VARCHAR(4000) NOT NULL,
CICX_MSG_TEXT VARCHAR(4000),
FAMILY_NAME VARCHAR(40) NOT NULL,
PAX_ID INTEGER NOT NULL,
POINT_ID INTEGER NOT NULL,
SEND_TIME TIMESTAMP NOT NULL,
SETTINGS_ID INTEGER,
STATUS VARCHAR(1)
);
