ALTER TABLE VALUE_BAG_TAXES ADD CONSTRAINT VALUE_BAG_TAXES__CITY_DEP__FK FOREIGN KEY (CITY_DEP) REFERENCES CITIES (CODE) ENABLE NOVALIDATE;