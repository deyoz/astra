ALTER TABLE COMP_LAYER_RULES ADD CONSTRAINT COMP_LAYER_RULES__SRC_LAYER__F FOREIGN KEY (SRC_LAYER) REFERENCES COMP_LAYER_TYPES (CODE) ;