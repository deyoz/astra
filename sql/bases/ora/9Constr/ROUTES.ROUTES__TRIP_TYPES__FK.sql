ALTER TABLE ROUTES ADD CONSTRAINT ROUTES__TRIP_TYPES__FK FOREIGN KEY (TRIP_TYPE) REFERENCES TRIP_TYPES (CODE) ENABLE NOVALIDATE;