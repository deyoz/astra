ALTER TABLE TLG_STAT ADD CONSTRAINT TLG_STAT__SENDER_COUNTRY__FK FOREIGN KEY (SENDER_COUNTRY) REFERENCES COUNTRIES (CODE) ;
