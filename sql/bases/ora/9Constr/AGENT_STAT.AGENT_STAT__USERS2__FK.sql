ALTER TABLE AGENT_STAT ADD CONSTRAINT AGENT_STAT__USERS2__FK FOREIGN KEY (USER_ID) REFERENCES USERS2 (USER_ID) ENABLE NOVALIDATE;
