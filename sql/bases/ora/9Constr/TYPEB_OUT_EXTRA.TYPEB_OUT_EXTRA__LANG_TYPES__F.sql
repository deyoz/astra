ALTER TABLE TYPEB_OUT_EXTRA ADD CONSTRAINT TYPEB_OUT_EXTRA__LANG_TYPES__F FOREIGN KEY (LANG) REFERENCES LANG_TYPES (CODE) ENABLE NOVALIDATE;
