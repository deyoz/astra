ALTER TABLE POS_TERM_ASSIGN ADD CONSTRAINT POS_TERM_ASSIGN__POS_TERM_SETS FOREIGN KEY (POS_ID) REFERENCES POS_TERM_SETS (ID);
