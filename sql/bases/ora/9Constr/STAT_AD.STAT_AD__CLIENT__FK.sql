ALTER TABLE STAT_AD ADD CONSTRAINT STAT_AD__CLIENT__FK FOREIGN KEY (CLIENT_TYPE) REFERENCES CLIENT_TYPES (CODE) ENABLE NOVALIDATE;
