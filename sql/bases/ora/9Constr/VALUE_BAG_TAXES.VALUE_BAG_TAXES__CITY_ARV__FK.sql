ALTER TABLE VALUE_BAG_TAXES ADD CONSTRAINT VALUE_BAG_TAXES__CITY_ARV__FK FOREIGN KEY (CITY_ARV) REFERENCES CITIES (CODE) ENABLE NOVALIDATE;
