ALTER TABLE CRS_PAX_FQT ADD CONSTRAINT CRS_PAX_FQT__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;