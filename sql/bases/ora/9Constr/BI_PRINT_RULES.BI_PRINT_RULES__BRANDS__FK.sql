ALTER TABLE BI_PRINT_RULES ADD CONSTRAINT BI_PRINT_RULES__BRANDS__FK FOREIGN KEY (BRAND_AIRLINE,BRAND_CODE) REFERENCES BRANDS (AIRLINE,CODE) ENABLE NOVALIDATE;
