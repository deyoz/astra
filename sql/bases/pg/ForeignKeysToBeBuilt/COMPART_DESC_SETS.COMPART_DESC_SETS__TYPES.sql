ALTER TABLE COMPART_DESC_SETS ADD CONSTRAINT COMPART_DESC_SETS__TYPES FOREIGN KEY (DESC_CODE) REFERENCES COMPART_DESC_TYPES (CODE) ;
