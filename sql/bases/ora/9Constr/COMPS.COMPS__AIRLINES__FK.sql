ALTER TABLE COMPS ADD CONSTRAINT COMPS__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
