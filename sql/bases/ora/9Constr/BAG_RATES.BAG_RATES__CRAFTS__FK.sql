ALTER TABLE BAG_RATES ADD CONSTRAINT BAG_RATES__CRAFTS__FK FOREIGN KEY (CRAFT) REFERENCES CRAFTS (CODE) ENABLE NOVALIDATE;
