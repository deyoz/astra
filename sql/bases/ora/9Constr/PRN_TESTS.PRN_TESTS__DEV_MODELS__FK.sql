ALTER TABLE PRN_TESTS ADD CONSTRAINT PRN_TESTS__DEV_MODELS__FK FOREIGN KEY (DEV_MODEL) REFERENCES DEV_MODELS (CODE) ENABLE NOVALIDATE;
