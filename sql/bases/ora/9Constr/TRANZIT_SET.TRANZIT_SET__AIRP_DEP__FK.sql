ALTER TABLE TRANZIT_SET ADD CONSTRAINT TRANZIT_SET__AIRP_DEP__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
