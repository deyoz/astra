ALTER TABLE CRS_PAX ADD CONSTRAINT CRS_PAX__ORIG_SUBCLASS__FK FOREIGN KEY (ORIG_SUBCLASS) REFERENCES SUBCLS (CODE) ENABLE NOVALIDATE;