ALTER TABLE CRS_PAX_DOCO ADD CONSTRAINT CRS_PAX_DOCO__PAX_DOC_TYPES__F FOREIGN KEY (TYPE) REFERENCES PAX_DOC_TYPES (CODE) ENABLE NOVALIDATE;
