ALTER TABLE BAG_PAY_TYPES ADD CONSTRAINT BAG_PAY_TYPES__PAY_TYPES__FK FOREIGN KEY (PAY_TYPE) REFERENCES PAY_TYPES (CODE) ;
