ALTER TABLE TYPEB_IN_BODY ADD CONSTRAINT TYPEB_IN_BODY__TYPEB_IN__FK FOREIGN KEY (ID,NUM) REFERENCES TLGS_IN (ID,NUM) ENABLE NOVALIDATE;