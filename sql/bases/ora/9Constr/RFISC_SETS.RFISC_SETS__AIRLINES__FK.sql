ALTER TABLE RFISC_SETS ADD CONSTRAINT RFISC_SETS__AIRLINES__FK FOREIGN KEY (AIRLINE) REFERENCES AIRLINES (CODE) ENABLE NOVALIDATE;
