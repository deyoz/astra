ALTER TABLE BP_SET ADD CONSTRAINT BP_SET__AIRP_DEP__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;