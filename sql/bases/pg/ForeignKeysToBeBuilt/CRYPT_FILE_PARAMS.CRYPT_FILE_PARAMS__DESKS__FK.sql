ALTER TABLE CRYPT_FILE_PARAMS ADD CONSTRAINT CRYPT_FILE_PARAMS__DESKS__FK FOREIGN KEY (DESK) REFERENCES DESKS (CODE) ;
