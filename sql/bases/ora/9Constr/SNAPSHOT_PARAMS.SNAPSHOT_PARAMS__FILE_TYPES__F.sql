ALTER TABLE SNAPSHOT_PARAMS ADD CONSTRAINT SNAPSHOT_PARAMS__FILE_TYPES__F FOREIGN KEY (FILE_TYPE) REFERENCES FILE_TYPES (CODE) ENABLE NOVALIDATE;
