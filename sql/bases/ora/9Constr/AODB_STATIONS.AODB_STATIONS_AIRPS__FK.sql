ALTER TABLE AODB_STATIONS ADD CONSTRAINT AODB_STATIONS_AIRPS__FK FOREIGN KEY (AIRP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;
