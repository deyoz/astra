ALTER TABLE COMP_BASELAYERS ADD CONSTRAINT COMP_BASELAYERS__COMP_ELEMS__F FOREIGN KEY (COMP_ID,NUM,X,Y) REFERENCES COMP_ELEMS (COMP_ID,NUM,X,Y) ;
