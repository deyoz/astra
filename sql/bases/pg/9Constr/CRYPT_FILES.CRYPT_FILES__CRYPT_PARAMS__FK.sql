ALTER TABLE CRYPT_FILES ADD CONSTRAINT CRYPT_FILES__CRYPT_PARAMS__FK FOREIGN KEY (PKCS_ID) REFERENCES CRYPT_FILE_PARAMS (PKCS_ID) ;
