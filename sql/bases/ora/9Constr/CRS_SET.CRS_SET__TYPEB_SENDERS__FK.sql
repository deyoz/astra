ALTER TABLE CRS_SET ADD CONSTRAINT CRS_SET__TYPEB_SENDERS__FK FOREIGN KEY (CRS) REFERENCES TYPEB_SENDERS (CODE) ENABLE NOVALIDATE;
