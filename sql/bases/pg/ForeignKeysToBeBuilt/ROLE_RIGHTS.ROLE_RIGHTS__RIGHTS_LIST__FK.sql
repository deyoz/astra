ALTER TABLE ROLE_RIGHTS ADD CONSTRAINT ROLE_RIGHTS__RIGHTS_LIST__FK FOREIGN KEY (RIGHT_ID) REFERENCES RIGHTS_LIST (IDA) ;
