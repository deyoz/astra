ALTER TABLE BAG_TAGS ADD CONSTRAINT BAG_TAGS__TAG_COLORS__FK FOREIGN KEY (COLOR) REFERENCES TAG_COLORS (CODE) ENABLE NOVALIDATE;
