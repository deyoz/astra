ALTER TABLE TRIP_CALC_DATA ADD CONSTRAINT TRIP_CALC_DATA__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ;