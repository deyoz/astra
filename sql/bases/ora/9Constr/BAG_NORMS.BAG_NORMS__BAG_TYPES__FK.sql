ALTER TABLE BAG_NORMS ADD CONSTRAINT BAG_NORMS__BAG_TYPES__FK FOREIGN KEY (BAG_TYPE) REFERENCES BAG_TYPES (CODE) ENABLE NOVALIDATE;