ALTER TABLE BAG2 ADD CONSTRAINT BAG2__BAG_TYPES__FK FOREIGN KEY (BAG_TYPE) REFERENCES BAG_TYPES (CODE) ENABLE NOVALIDATE;
