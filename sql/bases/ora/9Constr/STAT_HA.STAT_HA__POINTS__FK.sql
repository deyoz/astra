ALTER TABLE STAT_HA ADD CONSTRAINT STAT_HA__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ENABLE NOVALIDATE;