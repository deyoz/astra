ALTER TABLE SEAT_ALGO_SETS ADD CONSTRAINT SEAT_ALGO_SETS__AIRPS__FK FOREIGN KEY (AIRP_DEP) REFERENCES AIRPS (CODE) ENABLE NOVALIDATE;