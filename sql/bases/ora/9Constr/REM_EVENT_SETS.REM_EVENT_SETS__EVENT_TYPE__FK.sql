ALTER TABLE REM_EVENT_SETS ADD CONSTRAINT REM_EVENT_SETS__EVENT_TYPE__FK FOREIGN KEY (EVENT_TYPE) REFERENCES REM_EVENT_TYPES (CODE) ENABLE NOVALIDATE;