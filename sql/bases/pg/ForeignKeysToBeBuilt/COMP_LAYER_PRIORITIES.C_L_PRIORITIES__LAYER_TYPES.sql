ALTER TABLE COMP_LAYER_PRIORITIES ADD CONSTRAINT C_L_PRIORITIES__LAYER_TYPES FOREIGN KEY (LAYER_TYPE) REFERENCES COMP_LAYER_TYPES (CODE) ;