ALTER TABLE PRN_FORMS ADD CONSTRAINT PRN_FORMS__DEV_FMT_OPERS__FK FOREIGN KEY (OP_TYPE,FMT_TYPE) REFERENCES DEV_FMT_OPERS (OP_TYPE,FMT_TYPE) ENABLE NOVALIDATE;