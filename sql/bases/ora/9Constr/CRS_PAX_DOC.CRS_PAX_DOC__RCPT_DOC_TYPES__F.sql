ALTER TABLE CRS_PAX_DOC ADD CONSTRAINT CRS_PAX_DOC__RCPT_DOC_TYPES__F FOREIGN KEY (TYPE_RCPT) REFERENCES RCPT_DOC_TYPES (CODE) ENABLE NOVALIDATE;
