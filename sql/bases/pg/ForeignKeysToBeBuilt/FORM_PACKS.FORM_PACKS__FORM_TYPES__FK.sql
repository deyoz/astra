ALTER TABLE FORM_PACKS ADD CONSTRAINT FORM_PACKS__FORM_TYPES__FK FOREIGN KEY (TYPE) REFERENCES FORM_TYPES (CODE);
