ALTER TABLE TEST_PAX ADD CONSTRAINT TEST_PAX__SUBCLS__FK FOREIGN KEY (SUBCLASS) REFERENCES SUBCLS (CODE) ENABLE NOVALIDATE;