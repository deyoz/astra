ALTER TABLE CLS_GRP ADD CONSTRAINT CLS_GRP__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
