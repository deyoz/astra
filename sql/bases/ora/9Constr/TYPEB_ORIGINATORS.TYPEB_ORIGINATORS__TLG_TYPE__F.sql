ALTER TABLE TYPEB_ORIGINATORS ADD CONSTRAINT TYPEB_ORIGINATORS__TLG_TYPE__F FOREIGN KEY (TLG_TYPE) REFERENCES TYPEB_TYPES (CODE) ENABLE NOVALIDATE;
