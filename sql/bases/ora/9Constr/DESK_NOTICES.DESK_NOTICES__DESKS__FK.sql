ALTER TABLE DESK_NOTICES ADD CONSTRAINT DESK_NOTICES__DESKS__FK FOREIGN KEY (DESK) REFERENCES DESKS (CODE) ENABLE NOVALIDATE;
