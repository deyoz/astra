ALTER TABLE MARK_TRIPS ADD CONSTRAINT MARK_TRIPS__AIRPS__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
