ALTER TABLE BP_TYPES ADD CONSTRAINT BP_TYPES__DEV_OPER_TYPES__FK FOREIGN KEY (OP_TYPE) REFERENCES DEV_OPER_TYPES (CODE) ENABLE NOVALIDATE;