ALTER TABLE PAX_EVENTS ADD CONSTRAINT PAX_EVENTS__PAX__FK FOREIGN KEY (PAX_ID) REFERENCES PAX (PAX_ID) ENABLE NOVALIDATE;
