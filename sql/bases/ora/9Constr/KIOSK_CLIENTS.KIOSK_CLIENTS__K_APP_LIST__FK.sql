ALTER TABLE KIOSK_CLIENTS ADD CONSTRAINT KIOSK_CLIENTS__K_APP_LIST__FK FOREIGN KEY (APP_ID) REFERENCES KIOSK_APP_LIST (ID) ENABLE NOVALIDATE;
