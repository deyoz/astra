ALTER TABLE TAG_RANGES2 ADD CONSTRAINT TAG_RANGES2__CLASSES__FK FOREIGN KEY (CLASS) REFERENCES CLASSES (CODE) ENABLE NOVALIDATE;
