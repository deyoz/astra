ALTER TABLE STAT_REPRINT ADD CONSTRAINT STAT_REPRINT__CLIENT_TYPES__FK FOREIGN KEY (CKIN_TYPE) REFERENCES CLIENT_TYPES (CODE) ;
