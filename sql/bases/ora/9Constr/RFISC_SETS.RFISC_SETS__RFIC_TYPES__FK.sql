ALTER TABLE RFISC_SETS ADD CONSTRAINT RFISC_SETS__RFIC_TYPES__FK FOREIGN KEY (RFIC) REFERENCES RFIC_TYPES (CODE) ENABLE NOVALIDATE;
