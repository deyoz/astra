ALTER TABLE SUBCLS ADD CONSTRAINT SUBCLS__CLASSES__FK FOREIGN KEY (CLASS) REFERENCES CLASSES (CODE) ENABLE NOVALIDATE;