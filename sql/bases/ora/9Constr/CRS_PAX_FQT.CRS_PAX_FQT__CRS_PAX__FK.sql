ALTER TABLE CRS_PAX_FQT ADD CONSTRAINT CRS_PAX_FQT__CRS_PAX__FK FOREIGN KEY (PAX_ID) REFERENCES CRS_PAX (PAX_ID) ENABLE NOVALIDATE;
