ALTER TABLE KIOSK_ALIASES ADD CONSTRAINT KIOSK_ALIASES__KIOSK_LANG__FK FOREIGN KEY (LANG_ID) REFERENCES KIOSK_LANG (ID) ENABLE NOVALIDATE;
