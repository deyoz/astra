ALTER TABLE CRS_DISPLACE2 ADD CONSTRAINT CRS_DISPLACE__CLASS_TLG__FK FOREIGN KEY (CLASS_TLG) REFERENCES CLASSES (CODE) ENABLE NOVALIDATE;
