ALTER TABLE DEV_MODEL_FMT_PARAMS ADD CONSTRAINT DEV_MODEL_FMT_PARAMS__FMT_TYPE FOREIGN KEY (FMT_TYPE) REFERENCES DEV_FMT_TYPES (CODE) ENABLE NOVALIDATE;
