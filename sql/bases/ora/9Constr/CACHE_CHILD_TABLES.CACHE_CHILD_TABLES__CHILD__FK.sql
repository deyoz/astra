ALTER TABLE CACHE_CHILD_TABLES ADD CONSTRAINT CACHE_CHILD_TABLES__CHILD__FK FOREIGN KEY (CACHE_CHILD) REFERENCES CACHE_TABLES (CODE) ENABLE NOVALIDATE;
