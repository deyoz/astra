ALTER TABLE CONVERT_AIRLINES ADD CONSTRAINT CONVERT_AIRLINES__AIRLINES__FK FOREIGN KEY (CODE_INTERNAL) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;