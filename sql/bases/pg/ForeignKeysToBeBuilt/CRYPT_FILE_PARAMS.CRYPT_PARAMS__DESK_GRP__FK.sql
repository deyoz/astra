ALTER TABLE CRYPT_FILE_PARAMS ADD CONSTRAINT CRYPT_PARAMS__DESK_GRP__FK FOREIGN KEY (DESK_GRP_ID) REFERENCES DESK_GRP (GRP_ID) ;