ALTER TABLE PAX_BRANDS ADD CONSTRAINT PAX_BRANDS__PAX__FK FOREIGN KEY (PAX_ID) REFERENCES PAX (PAX_ID) ENABLE NOVALIDATE;
