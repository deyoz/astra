ALTER TABLE PAX_GRP ADD CONSTRAINT PAX_GRP__GRP_STATUS_TYPES__FK FOREIGN KEY (STATUS) REFERENCES GRP_STATUS_TYPES (CODE) ENABLE NOVALIDATE;
