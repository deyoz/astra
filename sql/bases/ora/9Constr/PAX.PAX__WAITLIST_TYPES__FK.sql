ALTER TABLE PAX ADD CONSTRAINT PAX__WAITLIST_TYPES__FK FOREIGN KEY (WL_TYPE) REFERENCES WAITLIST_TYPES (CODE) ENABLE NOVALIDATE;
