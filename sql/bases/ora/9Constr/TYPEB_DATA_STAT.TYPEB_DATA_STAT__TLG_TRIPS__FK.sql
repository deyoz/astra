ALTER TABLE TYPEB_DATA_STAT ADD CONSTRAINT TYPEB_DATA_STAT__TLG_TRIPS__FK FOREIGN KEY (POINT_ID) REFERENCES TLG_TRIPS (POINT_ID) ENABLE NOVALIDATE;
