ALTER TABLE RFISC_RATES_SELF_CKIN ADD CONSTRAINT RFISCRATESSELFCKIN__AIRPDEP FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;