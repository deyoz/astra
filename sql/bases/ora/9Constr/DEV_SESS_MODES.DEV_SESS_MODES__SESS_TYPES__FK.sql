ALTER TABLE DEV_SESS_MODES ADD CONSTRAINT DEV_SESS_MODES__SESS_TYPES__FK FOREIGN KEY (SESS_TYPE) REFERENCES DEV_SESS_TYPES (CODE) ENABLE NOVALIDATE;
