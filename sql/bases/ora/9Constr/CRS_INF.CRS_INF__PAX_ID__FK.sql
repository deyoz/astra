ALTER TABLE CRS_INF ADD CONSTRAINT CRS_INF__PAX_ID__FK FOREIGN KEY (PAX_ID) REFERENCES CRS_PAX (PAX_ID) ENABLE NOVALIDATE;
