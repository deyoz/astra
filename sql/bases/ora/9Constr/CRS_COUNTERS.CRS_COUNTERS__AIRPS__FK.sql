ALTER TABLE CRS_COUNTERS ADD CONSTRAINT CRS_COUNTERS__AIRPS__FK FOREIGN KEY (AIRP_ARV) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
