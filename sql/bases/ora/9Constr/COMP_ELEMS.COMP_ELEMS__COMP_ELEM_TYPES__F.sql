ALTER TABLE COMP_ELEMS ADD CONSTRAINT COMP_ELEMS__COMP_ELEM_TYPES__F FOREIGN KEY (ELEM_TYPE) REFERENCES COMP_ELEM_TYPES (CODE) ENABLE NOVALIDATE;