ALTER TABLE DEV_FMT_OPERS ADD CONSTRAINT DEV_FMT_OPERS__OPER_TYPES__FK FOREIGN KEY (OP_TYPE) REFERENCES DEV_OPER_TYPES (CODE) ;
