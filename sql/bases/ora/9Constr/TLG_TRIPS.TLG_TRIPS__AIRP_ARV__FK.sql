ALTER TABLE TLG_TRIPS ADD CONSTRAINT TLG_TRIPS__AIRP_ARV__FK FOREIGN KEY (AIRP_ARV) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;