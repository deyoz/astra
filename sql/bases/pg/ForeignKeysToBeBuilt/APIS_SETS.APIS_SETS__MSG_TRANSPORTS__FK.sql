ALTER TABLE APIS_SETS ADD CONSTRAINT APIS_SETS__MSG_TRANSPORTS__FK FOREIGN KEY (TRANSPORT_TYPE) REFERENCES MSG_TRANSPORTS (CODE) ENABLE NOVALIDATE;