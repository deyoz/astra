ALTER TABLE KIOSK_ALIASES ADD CONSTRAINT K_ALIASES__K_ALIASES_LIST__FK FOREIGN KEY (NAME) REFERENCES KIOSK_ALIASES_LIST (CODE) ;