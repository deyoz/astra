ALTER TABLE SALE_POINTS ADD CONSTRAINT SALE_POINTS__VALIDATOR_TYPES__FK FOREIGN KEY (VALIDATOR) REFERENCES VALIDATOR_TYPES (CODE) ;