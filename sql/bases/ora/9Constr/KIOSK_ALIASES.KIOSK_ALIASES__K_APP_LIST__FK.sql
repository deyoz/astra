ALTER TABLE KIOSK_ALIASES ADD CONSTRAINT KIOSK_ALIASES__K_APP_LIST__FK FOREIGN KEY (APP_ID) REFERENCES KIOSK_APP_LIST (ID) ENABLE NOVALIDATE;