ALTER TABLE TRIP_STATIONS ADD CONSTRAINT TRIP_STATIONS__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ;