ALTER TABLE CONFIRM_PRINT ADD CONSTRAINT CONFIRM_PRINT__VOUCHERS__FK FOREIGN KEY (VOUCHER) REFERENCES VOUCHER_TYPES (CODE) ENABLE NOVALIDATE;
