ALTER TABLE PAX_NORMS ADD CONSTRAINT PAX_NORMS__PAX__FK FOREIGN KEY (PAX_ID) REFERENCES PAX (PAX_ID) ENABLE NOVALIDATE;