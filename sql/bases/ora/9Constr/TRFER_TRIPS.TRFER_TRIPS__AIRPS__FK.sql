ALTER TABLE TRFER_TRIPS ADD CONSTRAINT TRFER_TRIPS__AIRPS__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
