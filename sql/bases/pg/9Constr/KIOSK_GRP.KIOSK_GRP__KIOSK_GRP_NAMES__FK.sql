ALTER TABLE KIOSK_GRP ADD CONSTRAINT KIOSK_GRP__KIOSK_GRP_NAMES__FK FOREIGN KEY (GRP_ID) REFERENCES KIOSK_GRP_NAMES (ID) ;