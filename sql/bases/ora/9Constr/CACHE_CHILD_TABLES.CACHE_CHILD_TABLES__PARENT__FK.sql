ALTER TABLE CACHE_CHILD_TABLES ADD CONSTRAINT CACHE_CHILD_TABLES__PARENT__FK FOREIGN KEY (CACHE_PARENT) REFERENCES CACHE_TABLES (CODE) ENABLE NOVALIDATE;
