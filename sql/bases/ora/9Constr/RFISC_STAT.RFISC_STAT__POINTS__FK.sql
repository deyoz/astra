ALTER TABLE RFISC_STAT ADD CONSTRAINT RFISC_STAT__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ENABLE NOVALIDATE;