ALTER TABLE TRFER_GRP ADD CONSTRAINT TRFER_GRP__TLG_TRANSFER__FK FOREIGN KEY (TRFER_ID) REFERENCES TLG_TRANSFER (TRFER_ID) ENABLE NOVALIDATE;
