ALTER TABLE CLS_GRP ADD CONSTRAINT CLS_GRP__CLASSES__FK FOREIGN KEY (CLASS) REFERENCES CLASSES (CODE) ENABLE NOVALIDATE;
