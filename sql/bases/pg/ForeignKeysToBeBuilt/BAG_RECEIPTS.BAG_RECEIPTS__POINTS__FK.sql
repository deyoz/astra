ALTER TABLE BAG_RECEIPTS ADD CONSTRAINT BAG_RECEIPTS__POINTS__FK FOREIGN KEY (POINT_ID) REFERENCES POINTS (POINT_ID) ;
