ALTER TABLE CONFIRMATION_SETS ADD CONSTRAINT CONFIRMATION_SETS__DCS_ACTIONS FOREIGN KEY (DCS_ACTION) REFERENCES DCS_ACTIONS (CODE) ENABLE NOVALIDATE;