ALTER TABLE RFISC_STAT ADD CONSTRAINT RFISC_STAT__TRFER_AIRP_ARV__FK FOREIGN KEY (TRFER_AIRP_ARV) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;