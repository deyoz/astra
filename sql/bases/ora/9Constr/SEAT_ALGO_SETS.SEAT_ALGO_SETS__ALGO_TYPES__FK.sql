ALTER TABLE SEAT_ALGO_SETS ADD CONSTRAINT SEAT_ALGO_SETS__ALGO_TYPES__FK FOREIGN KEY (ALGO_TYPE) REFERENCES SEAT_ALGO_TYPES (ID) ENABLE NOVALIDATE;
