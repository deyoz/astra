ALTER TABLE CRS_PAX_ALARMS ADD CONSTRAINT CRS_PAX_ALARMS__CRS_PAX__FK FOREIGN KEY (PAX_ID) REFERENCES CRS_PAX (PAX_ID) ENABLE NOVALIDATE;
