ALTER TABLE CRS_PNR ADD CONSTRAINT CRS_PNR__CLASSES__FK FOREIGN KEY (CLASS) REFERENCES CLASSES (CODE) ENABLE NOVALIDATE;
