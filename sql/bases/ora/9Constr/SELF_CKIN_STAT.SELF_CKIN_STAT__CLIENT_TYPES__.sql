ALTER TABLE SELF_CKIN_STAT ADD CONSTRAINT SELF_CKIN_STAT__CLIENT_TYPES__ FOREIGN KEY (CLIENT_TYPE) REFERENCES CLIENT_TYPES (CODE) ENABLE NOVALIDATE;
