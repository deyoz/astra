ALTER TABLE HALL_SET ADD CONSTRAINT HALL_SET__MISC_SET_TYPES__FK FOREIGN KEY (TYPE) REFERENCES MISC_SET_TYPES (CODE) ENABLE NOVALIDATE;