ALTER TABLE STAT_HA ADD CONSTRAINT STAT_HA__HOTEL__FK FOREIGN KEY (HOTEL_ID) REFERENCES HOTEL_ACMD (ID) ENABLE NOVALIDATE;
