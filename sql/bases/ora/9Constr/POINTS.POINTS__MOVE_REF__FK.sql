ALTER TABLE POINTS ADD CONSTRAINT POINTS__MOVE_REF__FK FOREIGN KEY (MOVE_ID) REFERENCES MOVE_REF (MOVE_ID) ENABLE NOVALIDATE;
