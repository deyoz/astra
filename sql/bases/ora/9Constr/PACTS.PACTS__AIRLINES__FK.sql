ALTER TABLE PACTS ADD CONSTRAINT PACTS__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
