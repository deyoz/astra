ALTER TABLE TRIP_BT ADD CONSTRAINT TRIP_BT__TAG_TYPES__FK FOREIGN KEY (TAG_TYPE) REFERENCES TAG_TYPES (CODE) ENABLE NOVALIDATE;