ALTER TABLE APIS_CUSTOMS ADD CONSTRAINT APIS_CUSTOMS__COUNTRY_REGUL__F FOREIGN KEY (COUNTRY_REGUL) REFERENCES COUNTRIES (CODE) ENABLE NOVALIDATE;