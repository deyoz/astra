ALTER TABLE PAX ADD CONSTRAINT PAX__CABIN_CLASS_GRP__FK FOREIGN KEY (CABIN_CLASS_GRP) REFERENCES CLS_GRP (ID) ENABLE NOVALIDATE;
