ALTER TABLE CUSTOM_ALARM_SETS ADD CONSTRAINT CUSTOM_ALARM_SETS__FQT_TIER_LE FOREIGN KEY (FQT_AIRLINE,FQT_TIER_LEVEL) REFERENCES FQT_TIER_LEVELS (AIRLINE,TIER_LEVEL) ENABLE NOVALIDATE;