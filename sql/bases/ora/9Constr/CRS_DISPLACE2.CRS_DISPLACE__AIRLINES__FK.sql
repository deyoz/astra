ALTER TABLE CRS_DISPLACE2 ADD CONSTRAINT CRS_DISPLACE__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;