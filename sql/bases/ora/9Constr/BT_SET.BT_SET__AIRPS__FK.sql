ALTER TABLE BT_SET ADD CONSTRAINT BT_SET__AIRPS__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
