ALTER TABLE CRS_PNR ADD CONSTRAINT CRS_PNR__SUBCLS__FK FOREIGN KEY (SUBCLASS) REFERENCES SUBCLS (CODE) ENABLE NOVALIDATE;