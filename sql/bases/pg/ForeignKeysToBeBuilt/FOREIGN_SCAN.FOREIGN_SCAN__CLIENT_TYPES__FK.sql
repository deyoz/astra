ALTER TABLE FOREIGN_SCAN ADD CONSTRAINT FOREIGN_SCAN__CLIENT_TYPES__FK FOREIGN KEY (CLIENT_TYPE) REFERENCES CLIENT_TYPES (CODE) ENABLE NOVALIDATE;