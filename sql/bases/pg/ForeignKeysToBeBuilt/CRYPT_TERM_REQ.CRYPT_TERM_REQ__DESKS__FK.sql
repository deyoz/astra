ALTER TABLE CRYPT_TERM_REQ ADD CONSTRAINT CRYPT_TERM_REQ__DESKS__FK FOREIGN KEY (DESK) REFERENCES DESKS (CODE) ;
