ALTER TABLE TRIP_TASKS ADD CONSTRAINT TRIP_TASKS__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ;
