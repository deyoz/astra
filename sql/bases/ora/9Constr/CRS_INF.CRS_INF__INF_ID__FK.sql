ALTER TABLE CRS_INF ADD CONSTRAINT CRS_INF__INF_ID__FK FOREIGN KEY (INF_ID) REFERENCES CRS_PAX (PAX_ID) ENABLE NOVALIDATE;
