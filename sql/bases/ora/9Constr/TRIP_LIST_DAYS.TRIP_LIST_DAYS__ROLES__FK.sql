ALTER TABLE TRIP_LIST_DAYS ADD CONSTRAINT TRIP_LIST_DAYS__ROLES__FK FOREIGN KEY (ROLE_ID) REFERENCES ROLES (ROLE_ID) ENABLE NOVALIDATE;