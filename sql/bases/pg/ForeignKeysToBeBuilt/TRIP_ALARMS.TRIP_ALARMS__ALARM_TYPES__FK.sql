ALTER TABLE TRIP_ALARMS ADD CONSTRAINT TRIP_ALARMS__ALARM_TYPES__FK FOREIGN KEY (ALARM_TYPE) REFERENCES ALARM_TYPES (CODE) ;