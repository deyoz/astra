ALTER TABLE BI_AIRLINE_SERVICE ADD CONSTRAINT BI_AIRLINE_SERVICE__TERMINAL__ FOREIGN KEY (TERMINAL) REFERENCES AIRP_TERMINALS (ID) ENABLE NOVALIDATE;
