ALTER TABLE PAX_GRP ADD CONSTRAINT PAX_GRP__CLASS__FK FOREIGN KEY (CLASS) REFERENCES CLASSES (CODE) ;