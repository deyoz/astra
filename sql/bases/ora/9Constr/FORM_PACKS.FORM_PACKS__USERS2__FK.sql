ALTER TABLE FORM_PACKS ADD CONSTRAINT FORM_PACKS__USERS2__FK FOREIGN KEY (USER_ID) REFERENCES USERS2 (USER_ID) ENABLE NOVALIDATE;
