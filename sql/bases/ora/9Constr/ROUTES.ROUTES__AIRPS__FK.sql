ALTER TABLE ROUTES ADD CONSTRAINT ROUTES__AIRPS__FK FOREIGN KEY (AIRP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;