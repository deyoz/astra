ALTER TABLE TYPEB_IN_ERRORS ADD CONSTRAINT TYPEB_IN_ERRORS__LANG_TYPES__F FOREIGN KEY (LANG) REFERENCES LANG_TYPES (CODE) ENABLE NOVALIDATE;
