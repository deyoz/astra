ALTER TABLE CONFIRMATION_SETS ADD CONSTRAINT CONFIRMATION_SETS__AIRPS__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
