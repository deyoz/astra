ALTER TABLE PAX ADD CONSTRAINT PAX__PAX_GRP__FK FOREIGN KEY (GRP_ID) REFERENCES PAX_GRP (GRP_ID) ENABLE NOVALIDATE;
