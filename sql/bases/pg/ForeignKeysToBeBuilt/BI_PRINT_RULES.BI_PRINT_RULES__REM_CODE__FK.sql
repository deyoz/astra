ALTER TABLE BI_PRINT_RULES ADD CONSTRAINT BI_PRINT_RULES__REM_CODE__FK FOREIGN KEY (REM_CODE) REFERENCES CKIN_REM_TYPES (CODE) ;
