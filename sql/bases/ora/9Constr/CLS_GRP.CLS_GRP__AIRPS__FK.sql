ALTER TABLE CLS_GRP ADD CONSTRAINT CLS_GRP__AIRPS__FK FOREIGN KEY (AIRP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;