ALTER TABLE MISC_SET ADD CONSTRAINT MISC_SET__MISC_SET_TYPES__FK FOREIGN KEY (TYPE) REFERENCES MISC_SET_TYPES (CODE) ENABLE NOVALIDATE;