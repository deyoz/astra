ALTER TABLE PAX_REM_ORIGIN ADD CONSTRAINT PAX_REM_ORIGIN__PAX__FK FOREIGN KEY (PAX_ID) REFERENCES PAX (PAX_ID) ENABLE NOVALIDATE;
