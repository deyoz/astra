ALTER TABLE PAX_SERVICES_AUTO ADD CONSTRAINT PAX_SERVICES_AUTO__PAX__FK FOREIGN KEY (PAX_ID) REFERENCES PAX (PAX_ID) ENABLE NOVALIDATE;
