ALTER TABLE TRFER_TAGS ADD CONSTRAINT TRFER_TAGS__TRFER_GRP__FK FOREIGN KEY (GRP_ID) REFERENCES TRFER_GRP (GRP_ID) ;