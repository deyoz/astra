ALTER TABLE STAT_SERVICES ADD CONSTRAINT STAT_SERVICES__AIRP_DEP__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
