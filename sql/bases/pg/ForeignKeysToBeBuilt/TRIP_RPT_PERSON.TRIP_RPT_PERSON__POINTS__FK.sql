ALTER TABLE TRIP_RPT_PERSON ADD CONSTRAINT TRIP_RPT_PERSON__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ;
