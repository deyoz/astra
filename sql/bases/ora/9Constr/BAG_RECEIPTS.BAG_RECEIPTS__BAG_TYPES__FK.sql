ALTER TABLE BAG_RECEIPTS ADD CONSTRAINT BAG_RECEIPTS__BAG_TYPES__FK FOREIGN KEY (BAG_TYPE) REFERENCES BAG_TYPES (CODE) ENABLE NOVALIDATE;
