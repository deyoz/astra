ALTER TABLE RFISC_RATES_SELF_CKIN ADD CONSTRAINT RFISC_RATES_SELF_CKIN__AIRLINE FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;