ALTER TABLE BI_STAT ADD CONSTRAINT BI_STAT__HALL__FK FOREIGN KEY (HALL) REFERENCES BI_HALLS (ID) ENABLE NOVALIDATE;
