ALTER TABLE CRS_PAX_ASVC ADD CONSTRAINT CRS_PAX_ASVC__CRS_PAX__FK FOREIGN KEY (PAX_ID) REFERENCES CRS_PAX (PAX_ID) ;