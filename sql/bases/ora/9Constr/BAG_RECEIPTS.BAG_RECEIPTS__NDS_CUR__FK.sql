ALTER TABLE BAG_RECEIPTS ADD CONSTRAINT BAG_RECEIPTS__NDS_CUR__FK FOREIGN KEY (NDS_CUR) REFERENCES CURRENCY (CODE) ENABLE NOVALIDATE;
