ALTER TABLE TRIP_BT ADD CONSTRAINT TRIP_BT__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ENABLE NOVALIDATE;
