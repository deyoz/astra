ALTER TABLE PAX_NORMS_TEXT ADD CONSTRAINT PAX_NORMS_TEXT__LANG_TYPES__FK FOREIGN KEY (LANG) REFERENCES LANG_TYPES (CODE) ENABLE NOVALIDATE;
