ALTER TABLE PAX_SEATS ADD CONSTRAINT PAX_SEATS__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ENABLE NOVALIDATE;
