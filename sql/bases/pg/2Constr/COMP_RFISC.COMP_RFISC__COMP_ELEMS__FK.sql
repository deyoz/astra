ALTER TABLE COMP_RFISC ADD CONSTRAINT COMP_RFISC__COMP_ELEMS__FK FOREIGN KEY (COMP_ID,NUM,X,Y) REFERENCES COMP_ELEMS (COMP_ID,NUM,X,Y) ;
