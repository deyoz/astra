ALTER TABLE CACHE_FIELDS ADD CONSTRAINT CACHE_FIELDS__CACHE_TABLES__FK FOREIGN KEY (CODE) REFERENCES CACHE_TABLES (CODE) ENABLE NOVALIDATE;
