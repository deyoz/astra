ALTER TABLE TRIP_DELAYS ADD CONSTRAINT TRIP_DELAYS__DELAYS__FK FOREIGN KEY (DELAY_CODE) REFERENCES DELAYS (CODE) ENABLE NOVALIDATE;