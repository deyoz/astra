ALTER TABLE BT_SET ADD CONSTRAINT BT_SET__TAG_TYPES__FK FOREIGN KEY (TAG_TYPE) REFERENCES TAG_TYPES (CODE) ENABLE NOVALIDATE;