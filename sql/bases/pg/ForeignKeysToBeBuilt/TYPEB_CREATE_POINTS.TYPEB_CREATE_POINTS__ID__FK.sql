ALTER TABLE TYPEB_CREATE_POINTS ADD CONSTRAINT TYPEB_CREATE_POINTS__ID__FK FOREIGN KEY (TYPEB_ADDRS_ID) REFERENCES TYPEB_ADDRS (ID);