ALTER TABLE RFISC_RATES ADD CONSTRAINT RFISC_RATES__BRANDS__FK FOREIGN KEY (AIRLINE,BRAND) REFERENCES BRANDS (AIRLINE,CODE) ENABLE NOVALIDATE;