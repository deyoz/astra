ALTER TABLE CRS_DISPLACE2 ADD CONSTRAINT CRS_DISPLACE__POINTS__FK FOREIGN KEY (POINT_ID_SPP) REFERENCES POINTS (POINT_ID) ;
