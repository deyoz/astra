ALTER TABLE BI_STAT ADD CONSTRAINT BI_STAT__DESK__FK FOREIGN KEY (DESK) REFERENCES DESKS (CODE) ENABLE NOVALIDATE;
