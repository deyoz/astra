ALTER TABLE WEB_CLIENTS ADD CONSTRAINT WEB_CLIENTS__CLIENT_TYPES__FK FOREIGN KEY (CLIENT_TYPE) REFERENCES CLIENT_TYPES (CODE) ENABLE NOVALIDATE;
