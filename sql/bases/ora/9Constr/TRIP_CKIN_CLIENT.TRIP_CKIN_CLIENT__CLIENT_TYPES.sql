ALTER TABLE TRIP_CKIN_CLIENT ADD CONSTRAINT TRIP_CKIN_CLIENT__CLIENT_TYPES FOREIGN KEY (CLIENT_TYPE) REFERENCES CLIENT_TYPES (CODE) ENABLE NOVALIDATE;
