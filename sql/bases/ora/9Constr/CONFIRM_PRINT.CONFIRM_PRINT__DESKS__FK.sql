ALTER TABLE CONFIRM_PRINT ADD CONSTRAINT CONFIRM_PRINT__DESKS__FK FOREIGN KEY (DESK) REFERENCES DESKS (CODE) ENABLE NOVALIDATE;
