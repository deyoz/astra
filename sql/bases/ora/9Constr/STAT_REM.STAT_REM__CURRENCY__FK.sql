ALTER TABLE STAT_REM ADD CONSTRAINT STAT_REM__CURRENCY__FK FOREIGN KEY (RATE_CUR) REFERENCES CURRENCY (CODE) ENABLE NOVALIDATE;