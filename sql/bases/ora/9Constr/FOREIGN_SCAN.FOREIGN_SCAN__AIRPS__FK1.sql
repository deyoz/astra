ALTER TABLE FOREIGN_SCAN ADD CONSTRAINT FOREIGN_SCAN__AIRPS__FK1 FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;