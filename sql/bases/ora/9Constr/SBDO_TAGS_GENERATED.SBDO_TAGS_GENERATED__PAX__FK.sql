ALTER TABLE SBDO_TAGS_GENERATED ADD CONSTRAINT SBDO_TAGS_GENERATED__PAX__FK FOREIGN KEY (PAX_ID) REFERENCES PAX (PAX_ID) ENABLE NOVALIDATE;
