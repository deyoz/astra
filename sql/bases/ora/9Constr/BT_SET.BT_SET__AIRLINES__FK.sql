ALTER TABLE BT_SET ADD CONSTRAINT BT_SET__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
