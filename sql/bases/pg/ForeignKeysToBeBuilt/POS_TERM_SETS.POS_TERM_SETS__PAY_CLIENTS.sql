ALTER TABLE POS_TERM_SETS ADD CONSTRAINT POS_TERM_SETS__PAY_CLIENTS FOREIGN KEY (CLIENT_ID) REFERENCES PAY_CLIENTS (ID);
