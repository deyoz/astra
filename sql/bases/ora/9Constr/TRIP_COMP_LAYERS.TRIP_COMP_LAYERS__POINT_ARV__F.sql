ALTER TABLE TRIP_COMP_LAYERS ADD CONSTRAINT TRIP_COMP_LAYERS__POINT_ARV__F FOREIGN KEY (POINT_ARV) REFERENCES POINTS (POINT_ID) ENABLE NOVALIDATE;