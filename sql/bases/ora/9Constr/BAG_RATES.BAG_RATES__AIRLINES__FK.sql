ALTER TABLE BAG_RATES ADD CONSTRAINT BAG_RATES__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
