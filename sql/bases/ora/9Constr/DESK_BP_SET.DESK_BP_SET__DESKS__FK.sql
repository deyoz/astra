ALTER TABLE DESK_BP_SET ADD CONSTRAINT DESK_BP_SET__DESKS__FK FOREIGN KEY (DESK) REFERENCES DESKS (CODE) ENABLE NOVALIDATE;