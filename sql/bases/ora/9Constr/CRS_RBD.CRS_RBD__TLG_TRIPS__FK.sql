ALTER TABLE CRS_RBD ADD CONSTRAINT CRS_RBD__TLG_TRIPS__FK FOREIGN KEY (POINT_ID) REFERENCES TLG_TRIPS (POINT_ID) ENABLE NOVALIDATE;
