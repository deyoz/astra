ALTER TABLE ROLE_ASSIGN_RIGHTS ADD CONSTRAINT ROLE_ASSIGN_RIGHTS__RIGHTS_LIS FOREIGN KEY (RIGHT_ID) REFERENCES RIGHTS_LIST (IDA) ENABLE NOVALIDATE;
