ALTER TABLE TYPEB_OPTION_VALUES ADD CONSTRAINT TYPEB_OPTION_VALUES__OPTIONS__ FOREIGN KEY (TLG_TYPE,CATEGORY) REFERENCES TYPEB_OPTIONS (TLG_TYPE,CATEGORY);