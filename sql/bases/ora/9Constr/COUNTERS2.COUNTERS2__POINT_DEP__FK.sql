ALTER TABLE COUNTERS2 ADD CONSTRAINT COUNTERS2__POINT_DEP__FK FOREIGN KEY (POINT_DEP) REFERENCES POINTS (POINT_ID) ENABLE NOVALIDATE;
