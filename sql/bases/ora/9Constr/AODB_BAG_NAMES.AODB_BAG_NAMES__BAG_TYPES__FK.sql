ALTER TABLE AODB_BAG_NAMES ADD CONSTRAINT AODB_BAG_NAMES__BAG_TYPES__FK FOREIGN KEY (BAG_TYPE) REFERENCES BAG_TYPES (CODE) ENABLE NOVALIDATE;
