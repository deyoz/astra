ALTER TABLE COMP_BASELAYERS ADD CONSTRAINT COMP_BASELAYERS__LAYER_TYPES__ FOREIGN KEY (LAYER_TYPE) REFERENCES COMP_LAYER_TYPES (CODE) ENABLE NOVALIDATE;
