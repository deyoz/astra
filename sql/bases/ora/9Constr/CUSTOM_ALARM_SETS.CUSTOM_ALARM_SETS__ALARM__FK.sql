ALTER TABLE CUSTOM_ALARM_SETS ADD CONSTRAINT CUSTOM_ALARM_SETS__ALARM__FK FOREIGN KEY (ALARM) REFERENCES CUSTOM_ALARM_TYPES (ID) ENABLE NOVALIDATE;
