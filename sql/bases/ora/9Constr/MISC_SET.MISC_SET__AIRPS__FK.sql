ALTER TABLE MISC_SET ADD CONSTRAINT MISC_SET__AIRPS__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
