ALTER TABLE COMP_REM ADD CONSTRAINT COMP_REM__COMP_ELEMS__FK FOREIGN KEY (COMP_ID,NUM,X,Y) REFERENCES COMP_ELEMS (COMP_ID,NUM,X,Y) ;