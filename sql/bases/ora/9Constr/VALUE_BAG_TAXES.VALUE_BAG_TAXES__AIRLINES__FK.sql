ALTER TABLE VALUE_BAG_TAXES ADD CONSTRAINT VALUE_BAG_TAXES__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
