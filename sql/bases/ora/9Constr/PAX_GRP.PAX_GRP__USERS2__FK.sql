ALTER TABLE PAX_GRP ADD CONSTRAINT PAX_GRP__USERS2__FK FOREIGN KEY (USER_ID) REFERENCES USERS2 (USER_ID) ENABLE NOVALIDATE;