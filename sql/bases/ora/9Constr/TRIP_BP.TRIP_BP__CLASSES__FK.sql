ALTER TABLE TRIP_BP ADD CONSTRAINT TRIP_BP__CLASSES__FK FOREIGN KEY (CLASS) REFERENCES CLASSES (CODE) ENABLE NOVALIDATE;