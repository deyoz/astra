ALTER TABLE KUF_STAT_AIRPS ADD CONSTRAINT KSA_AIRPS__FK FOREIGN KEY (AIRP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
