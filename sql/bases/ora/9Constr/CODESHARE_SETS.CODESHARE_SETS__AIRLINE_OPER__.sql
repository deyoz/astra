ALTER TABLE CODESHARE_SETS ADD CONSTRAINT CODESHARE_SETS__AIRLINE_OPER__ FOREIGN KEY (AIRLINE_OPER) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
