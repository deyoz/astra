ALTER TABLE PAY_METHODS_SET ADD CONSTRAINT PAY_METHODS_SET__DESK_GRP__FK FOREIGN KEY (DESK_GRP_ID) REFERENCES DESK_GRP (GRP_ID) ENABLE NOVALIDATE;