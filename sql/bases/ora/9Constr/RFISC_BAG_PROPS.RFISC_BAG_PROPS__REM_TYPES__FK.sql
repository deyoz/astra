ALTER TABLE RFISC_BAG_PROPS ADD CONSTRAINT RFISC_BAG_PROPS__REM_TYPES__FK FOREIGN KEY (REM_CODE_LCI) REFERENCES CKIN_REM_TYPES (CODE) ENABLE NOVALIDATE;
