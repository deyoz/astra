ALTER TABLE BAG_NORMS ADD CONSTRAINT BAG_NORMS__CLASSES__FK FOREIGN KEY (CLASS) REFERENCES CLASSES (CODE) ENABLE NOVALIDATE;