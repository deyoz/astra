ALTER TABLE BI_STAT ADD CONSTRAINT BI_STAT__PRINT_TYPE__FK FOREIGN KEY (PRINT_TYPE) REFERENCES BI_PRINT_TYPES (CODE) ;
