ALTER TABLE TRIP_SETS ADD CONSTRAINT TRIP_SETS__COMPS__FK FOREIGN KEY (COMP_ID) REFERENCES COMPS (COMP_ID) ENABLE NOVALIDATE;