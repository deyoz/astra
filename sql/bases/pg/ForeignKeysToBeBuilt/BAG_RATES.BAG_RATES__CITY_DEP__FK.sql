ALTER TABLE BAG_RATES ADD CONSTRAINT BAG_RATES__CITY_DEP__FK FOREIGN KEY (CITY_DEP) REFERENCES CITIES (CODE) ;