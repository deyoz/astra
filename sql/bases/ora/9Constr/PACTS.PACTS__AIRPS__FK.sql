ALTER TABLE PACTS ADD CONSTRAINT PACTS__AIRPS__FK FOREIGN KEY (AIRP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;