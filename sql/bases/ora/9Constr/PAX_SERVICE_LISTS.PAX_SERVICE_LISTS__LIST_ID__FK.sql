ALTER TABLE PAX_SERVICE_LISTS ADD CONSTRAINT PAX_SERVICE_LISTS__LIST_ID__FK FOREIGN KEY (LIST_ID) REFERENCES SERVICE_LISTS (ID) ENABLE NOVALIDATE;
