ALTER TABLE RFISC_RATES_SELF_CKIN ADD CONSTRAINT RFISC_RATES_SELF_CKIN__CURRENC FOREIGN KEY (RATE_CUR) REFERENCES CURRENCY (CODE) ENABLE NOVALIDATE;