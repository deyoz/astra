ALTER TABLE PAX_DOCA ADD CONSTRAINT PAX_DOCA__COUNTRY__FK FOREIGN KEY (COUNTRY) REFERENCES PAX_DOC_COUNTRIES (CODE) ENABLE NOVALIDATE;