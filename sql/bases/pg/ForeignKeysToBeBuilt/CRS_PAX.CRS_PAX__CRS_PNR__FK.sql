ALTER TABLE CRS_PAX ADD CONSTRAINT CRS_PAX__CRS_PNR__FK FOREIGN KEY (PNR_ID) REFERENCES CRS_PNR (PNR_ID) ;