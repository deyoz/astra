ALTER TABLE PAY_METHODS_SET ADD CONSTRAINT PAY_METHODS_SET__TYPES FOREIGN KEY (METHOD_TYPE) REFERENCES PAY_METHODS_TYPES (ID) ENABLE NOVALIDATE;
