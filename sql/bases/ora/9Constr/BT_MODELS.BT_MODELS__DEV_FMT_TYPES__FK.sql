ALTER TABLE BT_MODELS ADD CONSTRAINT BT_MODELS__DEV_FMT_TYPES__FK FOREIGN KEY (FMT_TYPE) REFERENCES DEV_FMT_TYPES (CODE) ENABLE NOVALIDATE;
