ALTER TABLE BAG_PREPAY ADD CONSTRAINT BAG_PREPAY__PAX_GRP__FK FOREIGN KEY (GRP_ID) REFERENCES PAX_GRP (GRP_ID) ;