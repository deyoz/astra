ALTER TABLE KIOSK_CONFIG ADD CONSTRAINT KIOSK_CONFIG__KIOSK_GRP__FK FOREIGN KEY (GRP_ID) REFERENCES KIOSK_GRP_NAMES (ID) ;
