ALTER TABLE PERS_WEIGHTS ADD CONSTRAINT PERS_WEIGHTS__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;