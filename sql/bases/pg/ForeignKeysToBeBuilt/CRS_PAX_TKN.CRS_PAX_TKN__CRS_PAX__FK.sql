ALTER TABLE CRS_PAX_TKN ADD CONSTRAINT CRS_PAX_TKN__CRS_PAX__FK FOREIGN KEY (PAX_ID) REFERENCES CRS_PAX (PAX_ID) ;
