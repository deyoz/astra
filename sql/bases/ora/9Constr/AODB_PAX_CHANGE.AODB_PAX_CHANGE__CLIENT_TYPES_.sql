ALTER TABLE AODB_PAX_CHANGE ADD CONSTRAINT AODB_PAX_CHANGE__CLIENT_TYPES_ FOREIGN KEY (CLIENT_TYPE) REFERENCES CLIENT_TYPES (CODE) ENABLE NOVALIDATE;